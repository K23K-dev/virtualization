#include "mmu.h"
#include "pagesim.h"
#include "address_splitting.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* The frame table pointer. You will set this up in system_init. */
fte_t *frame_table;

/**
 * ---------------- CHECKPOINT A: SYSTEM INITIALIZATION ----------------
 * 
 * In this problem, you will initialize the frame_table pointer. The frame table will
 * be located at physical address 0 in our simulated memory. You should zero out the 
 * entries in the frame table, in case for any reason physical memory is not clean.
 * 
 * HINTS:
 *      - mem: Simulated physical memory already allocated for you.
 *      - PAGE_SIZE: The size of one page
 * --------------------------------------------------------------------
 */

/*
 * Initializes the global frame table in frame 0 and protects it from
 * eviction. This establishes the bookkeeping used to manage physical RAM.
 */
void system_init(void) {
    // Initialize the frame table in frame 0 and protect it from eviction.
    frame_table = (fte_t *)mem;
    memset(frame_table, 0, PAGE_SIZE);
    frame_table[0].protected = 1;
}


/**
 * --------------- CHECKPOINT B: READING AND WRITING MEMORY ---------------
 * 
 * Takes an input virtual address and performs a memory operation.
 * 
 * @param addr virtual address to be translated
 * @param access   'r' if the access is a read, 'w' if a write
 * @param data If the access is a write, one byte of data to write to our memory.
 *             Ignored for read accesses.
 * 
 * @return The byte read on a read, or the byte written on a write.
 *
 * HINTS:
 *      - Remember that not all the entry in the process's page table are mapped in. 
 *      Check what in the pte_t struct signals that the entry is mapped in memory.
 * -----------------------------------------------------------------------
 */

/*
 * Handles a virtual-memory read or write for the current process.
 * Resolves missing pages, translates the address, performs the access,
 * and records reference and dirty bits for page replacement.
 * Returns the byte read or written.
 */
uint8_t mem_access(vaddr_t address, char access, uint8_t data) {
    stats.accesses++;
    
    // Find the mapping for this virtual address in the current process's page table.
    vpn_t vpn = get_vaddr_vpn(address);
    uint16_t offset = get_vaddr_offset(address);
    
    pte_t *entry = get_page_table_entry(vpn, PTBR, mem);

    // Bring the page into RAM if it is not currently resident.
    if (!entry->valid) {
        page_fault(address);
    }

    // Record this access for the replacement algorithms.
    entry->referenced = 1;

    paddr_t physical_address = get_physical_address(entry->pfn, offset);

    // Writes make the page dirty, so eviction will preserve its contents.
    if (access == 'w') {
        mem[physical_address] = data;
        entry->dirty = 1;
    }
    
    return mem[physical_address];
}
