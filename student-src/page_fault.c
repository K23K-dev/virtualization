#include "mmu.h"
#include "pagesim.h"
#include "swapops.h"
#include "stats.h"
#include "address_splitting.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * ----------------- CHECKPOINT B: PAGE FAULT HANDLING -----------------
 * 
 * Page fault handler.
 * 
 * When the CPU encounters an invalid address mapping in a page table, it invokes the 
 * OS via this handler. Your job is to put a mapping in place so that the translation 
 * can succeed.
 * 
 * @param addr virtual address in the page that needs to be mapped into main memory.
 * 
 * HINTS:
 *      - You will need to use the global variable current_process when
 *      altering the frame table entry.
 *      - Use swap_exists() and swap_read() to update the data in the 
 *      frame as it is mapped in.
 * ---------------------------------------------------------------------
 */

/*
 * Brings a missing virtual page into RAM so the pending access can
 * continue. Obtains a frame, restores its swap copy or zero-fills it,
 * and updates the page table and frame table to record the mapping.
 */
void page_fault(vaddr_t address) {
    stats.page_faults++;

    vpn_t vpn = get_vaddr_vpn(address);
    pte_t *entry = get_page_table_entry(vpn, PTBR, mem);

    // Obtain a frame, evicting another page if necessary.
    pfn_t pfn = free_frame();
    uint8_t *frame_start = mem + get_physical_address(pfn, 0); // frame's starting location

    // Restore saved contents, or zero-fill a new page.
    if (swap_exists(entry)) {
        swap_read(entry, frame_start);
    } else {
        memset(frame_start, 0, PAGE_SIZE);
    }

    // Map the page to this frame and mark it in RAM.
    // Clear dirty/referenced; mem_access updates them next.
    entry->pfn = pfn;
    entry->valid = 1;
    entry->dirty = 0;
    entry->referenced = 0;

    // Mark this frame occupied and record its owning process and virtual page.
    frame_table[pfn] = (fte_t) {
        .protected = 0,
        .mapped = 1,
        .ref_count = 0,
        .process = current_process,
        .vpn = vpn
    };
    
}

#pragma GCC diagnostic pop
