#include "proc.h"
#include "mmu.h"
#include "pagesim.h"
#include "address_splitting.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * ----------------- CHECKPOINT B: PROCESS INITIALIZATION -----------------
 * 
 * This function gets called every time a new process is created.
 * You will need to allocate a frame for the process's page table using the
 * free_frame function. Then, you will need update both the frame table and
 * the process's PCB. 
 * 
 * @param proc pointer to process that is being initialized 
 * 
 * HINTS:
 *      - pcb_t: struct defined in pagesim.h that is a process's PCB.
 *      - You are not guaranteed that the memory returned by the free frame allocator
 *      is empty - an existing frame could have been evicted for our new page table.
 * -----------------------------------------------------------------------
 */

/*
 * Creates an empty, protected page table for a new process and saves
 * its frame number in the PCB. This gives the process its own mappings;
 * its data pages are allocated later when accessed.
 */
void proc_init(pcb_t *proc) {
    // Allocate and clear a frame for the new process's page table.
    pfn_t pfn = free_frame();

    memset(get_page_table(pfn, mem), 0, PAGE_SIZE);

    // Protect the page table until the process exits.
    frame_table[pfn] = (fte_t) {
        .protected = 1,
        .mapped = 1,
        .ref_count = 0,
        .process = proc,
        .vpn = 0
    };

    // Save its frame number for future context switches.
    proc->saved_ptbr = pfn;
}


/**
 * ---- CHECKPOINT B: CONTEXT SWITCHES AND THE PAGE TABLE BASE REGISTER ----
 * 
 * Switches the currently running process to the process referenced by the proc 
 * argument.
 * 
 * Every process has its own page table, as you allocated in proc_init. You will
 * need to tell the processor to use the new process's page table.
 * 
 * @param proc pointer to process to become the currently running process.
 * 
 * HINTS:
 *      - Look at the global variables defined in pagesim.h. You may be interested in
 *      the definition of pcb_t as well.
 * --------------------------------------------------------------------------
 */

/*
 * Selects the current process and its page table so subsequent memory
 * accesses use that process's virtual address space.
 */
void context_switch(pcb_t *proc) {
    // Select this process's page table for subsequent memory accesses.
    PTBR = proc->saved_ptbr;
    current_process = proc;
}

/**
 * ------------------- CHECKPOINT B: FINISHING A PROCESS -------------------
 * 
 * When a process exits, you need to free any pages previously occupied by the
 * process.
 * 
 * HINTS:
 *      - If the process has swapped any pages to disk, you must call
 *      swap_free() using the page table entry pointer as a parameter.
 *      - If you free any protected pages, you must also clear their"protected" bits.
 * -------------------------------------------------------------------------
 */

/*
 * Releases an exiting process's resident pages, swap copies, and
 * page-table frame so those resources can be reused by other processes.
 * Also clears the current-process state if this process was running.
 */
void proc_cleanup(pcb_t *proc) {
    pfn_t ptbr = proc->saved_ptbr;
    pte_t *page_table = get_page_table(ptbr, mem);
    
    // Release each resident page and any saved swap copy.
    for (vpn_t vpn = 0; vpn < NUM_PAGES; vpn++) {
        pte_t *entry = &page_table[vpn];

        if (entry->valid) {
            frame_table[entry->pfn] = (fte_t){0};
        }

        if (swap_exists(entry)) {
            swap_free(entry);
        }

        *entry = (pte_t){0};
    }

    // Release the page-table frame, including its protection bit.
    frame_table[ptbr] = (fte_t){0};

    // Clear the current-process state if this process was running.
    if (current_process == proc) {
        current_process = NULL;
        PTBR = 0;
    }
}

#pragma GCC diagnostic pop
