#pragma once

#include "mmu.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * ------------------- CHECKPOINT A: ADDRESS SPLITTING -------------------
 *
 * Implement helper functions used to split virtual addresses and compute
 * page-table / physical-memory locations.
 * 
 * HINT: 
 *      - Examine the global defines in pagesim.h, which are necessary when
 *      implementing these functions.
 * ----------------------------------------------------------------------
 */

 /**
 * Virtual address → page number + offset
 * Page-table lookup → frame number
 * Frame number + offset → physical address
 */

/*
 * Extracts the virtual page number, which identifies the page-table
 * entry to use when translating a virtual address.
 */
static inline vpn_t get_vaddr_vpn(vaddr_t addr) {
    // Shift right by 14 to remove the offset bits to get the virtual page number.
    return (vpn_t)(addr >> OFFSET_LEN);
}

/*
 * Extracts the byte's position within its page. This offset stays
 * unchanged when the virtual page is translated to a physical frame.
 */
static inline uint16_t get_vaddr_offset(vaddr_t addr) {
    // Keep only the bits identifying the byte within the page.
    return (uint16_t)(addr & (PAGE_SIZE - 1));
}

/*
 * ptbr is the frame number holding the page table. Multiplying it by
 * PAGE_SIZE gives the table's byte offset in memory. The pte_t * cast
 * lets us treat those bytes as an array of page-table entries.
 */
static inline pte_t* get_page_table(pfn_t ptbr, uint8_t *memory) {
    // Convert the page-table frame number into a pointer inside simulated RAM.
    return (pte_t *)(memory + ((paddr_t)ptbr * PAGE_SIZE));
}

/*
 * Uses vpn as the index into the page table and returns a pointer to
 * that entry so we can read or update it. Its valid flag says whether
 * the page is in RAM; if valid, pfn identifies the frame holding its data.
 */
static inline pte_t* get_page_table_entry(vpn_t vpn, pfn_t ptbr, uint8_t *memory) {
    // Use the VPN as the index into this process's page table.
    return &get_page_table(ptbr, memory)[vpn];
}

/*
 * Combines a physical frame number and page offset to locate the
 * actual byte in simulated RAM.
 */
static inline paddr_t get_physical_address(pfn_t pfn, uint16_t offset) {
    // Add the page offset to the physical frame's starting address.
    return ((paddr_t)pfn * PAGE_SIZE) + offset;
}

#pragma GCC diagnostic pop
