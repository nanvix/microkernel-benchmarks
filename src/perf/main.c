/*
 * MIT License
 *
 * Copyright(c) 2018 Pedro Henrique Penna <pedrohenriquepenna@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <ulibc/stdio.h>
#include <nanvix.h>
#include <kbench.h>

/*============================================================================*
 * Profilling                                                                 *
 *============================================================================*/

/**
 * @brief Number of events to profile.
 */
#if defined(__mppa256__)
	#define BENCHMARK_PERF_EVENTS 7
#elif defined(__optimsoc__)
	#define BENCHMARK_PERF_EVENTS 5
#endif

/**
 * Performance events.
 */
static int perf_events[BENCHMARK_PERF_EVENTS] = {
#if defined(__mppa256__)
	PERF_CYCLES,
	PERF_ICACHE_STALLS,
	PERF_DCACHE_STALLS,
	PERF_BRANCH_STALLS,
	PERF_REG_STALLS,
	PERF_ITLB_STALLS,
	PERF_DTLB_STALLS
#elif defined(__optimsoc__)
	PERF_CYCLES,
	PERF_BRANCH_STALLS,
	PERF_ICACHE_STALLS,
	PERF_DCACHE_STALLS,
	PERF_REG_STALLS
#endif
};

/**
 * @brief Dump execution statistics.
 *
 * @param it    Benchmark iteration.
 * @param stats Execution statistics.
 */
static inline void benchmark_dump_stats(int it, uint64_t *stats)
{
	static spinlock_t lock = SPINLOCK_UNLOCKED;

	spinlock_lock(&lock);

		printf("%s %d %d %d %d %d %d %d %d\n",
			"[benchmarks][perf]",
			it,
			UINT32(stats[0]),
			UINT32(stats[1]),
			UINT32(stats[2]),
			UINT32(stats[3]),
			UINT32(stats[4]),
			UINT32(stats[5]),
			UINT32(stats[6])
		);

	spinlock_unlock(&lock);
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Performance Monitoring Overhead Benchmark
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int main(int argc, const char *argv[])
{
	uint64_t stats[BENCHMARK_PERF_EVENTS];

	((void) argc);
	((void) argv);

	printf(HLINE);

	/*
	 * TODO: Query performance monitoring capabilities.
	 */

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			perf_start(0, perf_events[j]);

				/* noop. */

			perf_stop(0);
			stats[j] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, stats);
	}

	printf(HLINE);

	return (0);
}
