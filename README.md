# Project 3: Virtual Memory

This project fills in the memory-management code of a C simulator. The provided driver starts processes, requests memory reads and writes, and stops processes. Our code translates addresses, handles missing pages and replacement, releases resources, and calculates memory-access costs.

[Official project instructions](https://github.gatech.edu/pages/CS2200TAs/Projects/Project-3/index.html)

## 1. The basic model

`mem` is a byte array representing **1 MiB of physical RAM**. A **page** is a chunk of virtual memory; a **frame** is an equally sized slot in RAM. Each is **16 KiB**, so RAM has **64 frames**.

```text
Virtual address:  [ 10-bit VPN | 14-bit offset ]
Physical address: [  6-bit PFN | 14-bit offset ]
```

Translation replaces the virtual page number (VPN) with a physical frame number (PFN). The byte offset stays the same.

| Structure | Purpose |
|---|---|
| Page table, one per process | Maps its 1,024 virtual pages to physical frames using page-table entries (PTEs). |
| Frame table, shared | Tracks each frame's owner, VPN, protection, and replacement history. |
| Process control block (PCB) | Stores process information, including its page-table frame in `saved_ptbr`. |

For example, `0x8005` has VPN **2** and offset **5**. If page 2 maps to frame 7, the physical address is `7 * 0x4000 + 5 = 0x1C005`.

## 2. What each implemented function does

### `student-src/address_splitting.h`

| Function | Job |
|---|---|
| `get_vaddr_vpn` | Shifts the address right by 14 to remove the offset. |
| `get_vaddr_offset` | Masks with `PAGE_SIZE - 1` to keep the lower 14 bits. |
| `get_page_table` | Locates `memory + ptbr * PAGE_SIZE` and casts it to `pte_t *`. |
| `get_page_table_entry` | Uses the VPN to index that page table. |
| `get_physical_address` | Computes `pfn * PAGE_SIZE + offset`. |

### `student-src/mmu.c`

- **`system_init`** places the frame table in frame 0, clears it, and protects that frame.
- **`mem_access`** counts the access, finds the PTE, handles a fault if needed, sets `referenced`, and translates the address. A write stores the byte and sets `dirty`. It returns the byte read or written.

### `student-src/proc.c`

- **`proc_init`** obtains and clears a page-table frame, marks it mapped and protected, and saves its PFN in the PCB. Data pages are allocated later on demand.
- **`context_switch`** sets `PTBR = proc->saved_ptbr` and `current_process = proc`.
- **`proc_cleanup`** releases resident frames and swap copies, clears PTEs, and releases the page-table frame. It also clears the current-process state if that process was running.

### `student-src/page_fault.c`

- **`page_fault`** counts the fault, obtains a frame, restores swap contents or zero-fills the frame, and installs the PTE and frame-table ownership information.

### `student-src/page_replacement.c`

- **`select_victim_frame`** prefers an unprotected free frame. Otherwise, it chooses a victim using the selected policy. Random replacement is provided starter logic; our implementation adds Clock Sweep and Approximate LRU.
- **`free_frame`** calls victim selection, saves a dirty victim, invalidates its old mapping, clears the frame's metadata, and returns its PFN.
- **`daemon_update`** periodically ages the reference histories used by Approximate LRU.

### `student-src/stats.c`

- **`compute_stats`** sets `stats.amat` from the access, page-fault, and writeback counts.

The provided driver and swap helpers handle trace input and storage of swapped pages.

## 3. One walkthrough for the demo

Suppose process A writes **42** to virtual address `0x8005`.

1. `proc_init` has allocated A's page table; `context_switch` selects it.
2. `mem_access` extracts VPN 2 and offset 5 and looks up A's PTE.
3. If the PTE is invalid, `page_fault` calls `free_frame`, which may evict another page.
4. The handler restores a swap copy or zero-fills the frame, then updates both tables.
5. `mem_access` writes 42 at the translated address and marks the page referenced and dirty.
6. If that page is later evicted, its dirty contents are saved to swap and its PTE becomes invalid.
7. A later read faults the page back in and returns 42, even if the page now occupies a different frame.

## 4. Likely TA questions and short answers

These are practice questions based on this implementation, not confirmed TA questions.

**1. What does PTBR contain? Does the pointer cast allocate memory?**

PTBR contains the page table's physical **frame number**, so I multiply it by `PAGE_SIZE` to find its byte location inside `mem`. The cast only interprets existing bytes as PTEs. Indexing that pointer advances by the size of a PTE.

**2. Why do you need both a page table and a frame table?**

The page table translates a process's VPN to a frame. The frame table provides the reverse ownership information. During eviction, I use the victim's process and VPN to find its PTE, rather than assuming it belongs to the current process.

**3. Can two processes use the same virtual address? What changes on a context switch?**

Yes, their separate page tables can map that address to different frames. A context switch changes `PTBR` and `current_process`; it does not copy or move the processes' data in RAM.

**4. What do `valid`, `dirty`, and `referenced` mean?**

`valid` means the page is resident in RAM. `dirty` means it was written since being loaded or initialized and must be saved if evicted. `referenced` records an access since that bit was last cleared. Reads and writes set `referenced`; only writes set `dirty`.

**5. Why protect some frames and zero newly allocated memory?**

Frame 0 and process page-table frames contain bookkeeping that replacement must not overwrite. Reused page-table memory is cleared so old bytes cannot appear to be valid mappings. Data pages without a swap copy are zero-filled so they cannot expose a previous owner's contents.

**6. Is an invalid PTE an error? What happens to the pending access?**

Here it means the page is not resident, so I handle a page fault and install its mapping. The existing PTE pointer still refers to the same entry in a protected page-table frame. `mem_access` then continues, sets `referenced`, and performs the read or write.

**7. How are selecting a victim and evicting it different?**

`select_victim_frame` chooses a PFN. `free_frame` performs any required writeback and invalidates the old mapping. Clearing its frame-table entry releases ownership but does not erase the frame's bytes; the new owner restores or clears them.

**8. Why preserve the swap ID, and can a clean page be evicted without writing?**

The PTE's `sid` locates its saved data after eviction. A clean page restored from swap can reuse that unchanged backup. A clean page with no backup has only zero-filled contents, so it can be zero-filled again. Only dirty eviction increments `writebacks`.

**9. How does your Clock Sweep work? Why allow two passes?**

The hand skips protected frames, clears a referenced page's bit, and selects an unreferenced page. One pass can clear all reference bits; the second can select an eligible frame. I advance the hand before returning, so `last_evicted` actually holds the next candidate position. If all frames are protected, selection reports exhaustion.

**10. How does Approximate LRU work, and why is it approximate?**

Every five memory requests, the daemon shifts each eligible frame's 8-bit history right, inserts its reference bit at the top using `0x80`, and clears that bit. For example, `10000000` becomes `11000000` if referenced again. The smallest history is selected, with ties going to the first frame encountered. Sampling loses the exact order and frequency of accesses within each interval.

**11. Why does cleanup check swap even when a PTE is invalid?**

An evicted page can still own a swap copy. I release both resident frames and all swap copies, then release the page-table frame and its protection. Exiting-process data does not need a writeback because the process will no longer use it.

**12. How do you calculate average memory access time?**

```text
AMAT = (accesses * 200 + page_faults * 150000
        + writebacks * 250000) / accesses
```

This is the project's cost model, including a fault cost even for zero-fill faults. `mem_access`, `page_fault`, and `free_frame` update the respective counters. I use floating-point arithmetic and set AMAT to zero if there were no accesses. For 100 accesses, two faults, and one writeback, AMAT is 5,700.
