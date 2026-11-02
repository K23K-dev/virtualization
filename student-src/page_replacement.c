#include "types.h"
#include "pagesim.h"
#include "mmu.h"
#include "swapops.h"
#include "stats.h"
#include "util.h"
#include "address_splitting.h"

pfn_t select_victim_frame(void);

pfn_t last_evicted = 0;

/**
 * --------- CHECKPOINT A: FREE FRAME SELECTION AND EVICTION ---------
 *
 * Make a free frame for the system to use. You call the select_victim_frame() method
 * to identify an "available" frame in the system (already given). You will need to
 * check to see if this frame is already mapped in, and if it is, you need to evict it.
 *
 * @return victim_pfn: a physical frame number to a free frame used by other functions.
 *
 * HINTS:
 *      - When evicting pages, remember what you checked for to trigger page faults
 *      in mem_access
 *      - If the page table entry has been written to before, you will need to use
 *      swap_write() to save the contents to the swap queue.
 * -------------------------------------------------------------------
 */

/*
 * Obtains a reusable physical frame for a page or page table.
 * If occupied, saves dirty contents to swap and invalidates the old
 * mapping before releasing the frame, preserving data for later reload.
 */
pfn_t free_frame(void) {
    pfn_t victim_pfn = select_victim_frame();
    fte_t *victim = &frame_table[victim_pfn];

    if (victim->mapped) {
        // Find the victim's PTE through its owner, which may be another process.
        pte_t *entry = get_page_table_entry(
            victim->vpn,
            victim->process->saved_ptbr,
            mem
        );

        // Save modified contents before reusing the frame.
        if (entry->dirty) {
            swap_write(
                entry,
                mem + get_physical_address(victim_pfn, 0)
            );
            stats.writebacks++;
        }

        // Invalidate the mapping but preserve its swap ID for a later reload.
        entry->valid = 0;
        entry->dirty = 0;
        entry->referenced = 0;
    }

    // Clear the frame table entry.
    *victim = (fte_t){0};

    return victim_pfn;
}

/**
 * ---------------- CHECKPOINT A: BETTER VICTIM SELECTION ----------------
 *
 * Finds a free physical frame. If none are available, uses either a
 * randomized, FCFS, or Second Chance (Clock Sweep) algorithm to find a used frame for
 * eviction.
 *
 * @return The physical frame number of a victim frame.
 *
 * HINTS:
 *      - Use the global variables MEM_SIZE and PAGE_SIZE to calculate
 *      the number of entries in the frame table.
 *      - Use the global last_evicted to keep track of the pointer into the frame table
 * ----------------------------------------------------------------------
 */

/*
 * Chooses an unprotected frame, preferring a free frame before using
 * the configured replacement policy. This decides which page gives
 * up its space when RAM is full; free_frame handles the actual eviction.
 */
pfn_t select_victim_frame()
{
    /* See if there are any free frames first */
    size_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < num_entries; i++)
    {
        if (!frame_table[i].protected && !frame_table[i].mapped)
        {
            return i;
        }
    }

    if (replacement == RANDOM)
    {
        /* Play Russian Roulette to decide which frame to evict */
        pfn_t unprotected_found = NUM_FRAMES;
        for (pfn_t i = 0; i < num_entries; i++)
        {
            if (!frame_table[i].protected)
            {
                unprotected_found = i;
                if (prng_rand() % 2)
                {
                    return i;
                }
            }
        }
        /* If no victim found yet take the last unprotected frame
           seen */
        if (unprotected_found < NUM_FRAMES)
        {
            return unprotected_found;
        }
    }
    else if (replacement == APPROX_LRU)
    {
        // Choose the unprotected frame with the smallest recent-access history.
        pfn_t victim = NUM_FRAMES;

        for (pfn_t i = 0; i < num_entries; i++) {
            if (frame_table[i].protected) {
                continue;
            }

            // Pick the first eligible frame, then keep the one with the smallest access history.
            if (victim == NUM_FRAMES || frame_table[i].ref_count < frame_table[victim].ref_count) {
                victim = i;
            }
        }

        if (victim < NUM_FRAMES) {
            return victim;
        }

    }
    else if (replacement == CLOCKSWEEP)
    {
        // Second chance replacement algorithm.
        for (size_t scanned = 0; scanned < 2 * num_entries; scanned++) {
            pfn_t candidate = last_evicted;

            // Advance the hand so the next search continues after this candidate.
            last_evicted = (pfn_t)((last_evicted + 1) % num_entries);

            fte_t *frame = &frame_table[candidate];

            if (frame->protected) {
                continue;
            }

            pte_t *entry = get_page_table_entry(
                frame->vpn,
                frame->process->saved_ptbr,
                mem
            );

            // Give referenced pages a second chance by clearing their reference bit.
            if (entry->referenced) {
                entry->referenced = 0;
            } else {
                return candidate;
            }
        }
    }

    // If every frame is protected, give up. This should never happen on the traces we provide you.
    panic("System ran out of memory\n");
    exit(1);
}
/**
 * ------------- CHECKPOINT B: DAEMON UPDATES FOR APPROXIMATE LRU -------------
 *
 * Updates the associated variables for the Approximate LRU,
 * called every time the simulator daemon wakes up.
 *
 * -------------------------------------------------------------------------------
 */

/*
 * Periodically samples page references and ages each eligible frame's
 * access history. Approximate LRU uses these histories to favor eviction
 * of pages that have been used less recently.
 */
void daemon_update(void)
{
    for (pfn_t pfn = 0; pfn < NUM_FRAMES; pfn++) {
        fte_t *frame = &frame_table[pfn];   // frame points to the current frame's frame-table entry

        // skip the frame if protected or unused.
        if (frame->protected || !frame->mapped) {
            continue;
        }

        // Find this frame's page-table entry using its owner and virtual page number.
        pte_t *entry = get_page_table_entry(
            frame->vpn,
            frame->process->saved_ptbr,
            mem
        );

        // Age the access history and insert this interval's reference bit.
        frame->ref_count >>= 1;
        
        if (entry->referenced) {
            frame->ref_count |= 0x80;   // set the leftmost bit to 1
        }

        // Clear the reference bit for the next sampling interval.
        entry->referenced = 0;
    }
}
