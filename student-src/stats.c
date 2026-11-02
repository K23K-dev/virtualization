#include "stats.h"

/* The stats. See the definition in stats.h. */
stats_t stats;

/**
 * ----------------- CHECKPOINT B: COMPUTING AMAT -----------------
 * 
 * Calculate the total average time it takes for an access.
 * 
 * HINTS:
 * 		- You may find the #defines in the stats.h file useful.
 * 		- You will need to include code to increment many of these stats in
 * 		the functions you have written for other parts of the project.
 * -----------------------------------------------------------------
 */

/*
 * Calculates average memory-access time from the project's modeled
 * access, page-fault, and writeback costs. Stores the result in
 * stats.amat, using zero when no memory accesses occurred.
 */
void compute_stats() {
	// Avoid dividing by zero when no memory accesses occurred.
	if (stats.accesses == 0) {
		stats.amat = 0.0;
		return;
	}

	// Average memory-access, page-fault, and writeback costs over all accesses.
	double total_time =
		(double)stats.accesses * MEMORY_ACCESS_TIME
		+ (double)stats.page_faults * DISK_PAGE_READ_TIME
		+ (double)stats.writebacks * DISK_PAGE_WRITE_TIME;

	stats.amat = total_time / (double)stats.accesses;
}
