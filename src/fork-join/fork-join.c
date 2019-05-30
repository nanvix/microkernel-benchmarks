/*
 * MIT License
 *
 * Copyright(c) 2011-2019 The Maintainers of Nanvix
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

#include <nanvix.h>
#include <stdint.h>
#include <kbench.h>

/**
 * @brief Number of events to profile.
 */
#define BENCHMARK_PERF_EVENTS 7

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                1  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               1  /**< Increment on Number of Working Threads */
/**@}*/

/**
 * Performance events.
 */
static int perf_events[BENCHMARK_PERF_EVENTS] = {
	PERF_CYCLES,
	PERF_ICACHE_STALLS,
	PERF_DCACHE_STALLS,
	PERF_BRANCH_STALLS,
	PERF_REG_STALLS,
	PERF_ITLB_STALLS,
	PERF_DTLB_STALLS
};

/**
 * @name Benchmark Kernel Parameters
 */
/**@{*/
static int NTHREADS; /**< Number of Working Threads */
/**@}*/

/**
 * @brief Dump execution statistics.
 *
 * @param it    Benchmark iteration.
 * @param stats Execution statistics.
 */
static inline void benchmark_dump_stats(int it, uint64_t *fork_stats, uint64_t *join_stats)
{
	printf("%s %d %s %d %d %d %d %d %d %d %d %d",
		"[benchmarks][fork-join]",
		it,
		"f",
		NTHREADS,
		UINT32(fork_stats[0]),
		UINT32(fork_stats[1]),
		UINT32(fork_stats[2]),
		UINT32(fork_stats[3]),
		UINT32(fork_stats[4]),
		UINT32(fork_stats[5]),
		UINT32(fork_stats[6]),
		UINT32(fork_stats[7])
	);

	printf("%s %d %s %d %d %d %d %d %d %d %d %d",
		"[benchmarks][fork-join]",
		it,
		"j",
		NTHREADS,
		UINT32(join_stats[0]),
		UINT32(join_stats[1]),
		UINT32(join_stats[2]),
		UINT32(join_stats[3]),
		UINT32(join_stats[4]),
		UINT32(join_stats[5]),
		UINT32(join_stats[6]),
		UINT32(join_stats[7])
	);
}

/**
 * @brief Dummy task.
 *
 * @param arg Unused argument.
 */
static void *task(void *arg)
{
	((void) arg);

	return (NULL);
}

/**
 * @brief Fork-Join Kernel
 *
 * @param nthreads Number of working threads.
 */
void kernel_fork_join(int nthreads)
{
	kthread_t tid[NTHREADS_MAX];
	uint64_t t0, t1, tmp;
	uint64_t fork_stats[BENCHMARK_PERF_EVENTS + 1];
	uint64_t join_stats[BENCHMARK_PERF_EVENTS + 1];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	fork_stats[0] = UINT64_MAX;
	join_stats[0] = UINT64_MAX;

	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			t0 = stopwatch_read();
			perf_start(0, perf_events[j]);

				/* Spawn threads. */
				for (int k = 0; k < nthreads; k++)
					kthread_create(&tid[k], task, NULL);

			perf_stop(0);
			t1 = stopwatch_read();

			tmp = stopwatch_diff(t0, t1);
			if (tmp < fork_stats[0])
				fork_stats[0] = tmp;
			fork_stats[j + 1] = perf_read(0);

			t0 = stopwatch_read();
			perf_start(0, perf_events[j]);

				/* Wait for threads. */
				for (int k = 0; k < nthreads; k++)
					kthread_join(tid[k], NULL);

			perf_stop(0);
			t1 = stopwatch_read();

			tmp = stopwatch_diff(t0, t1);
			if (tmp < join_stats[0])
				join_stats[0] = tmp;
			join_stats[j + 1] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, fork_stats, join_stats);
	}
}

/**
 * @brief Fork-Join Benchmark
 */
void benchmark_fork_join(void)
{
#ifndef NDEBUG

	kernel_fork_join(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_fork_join(nthreads);

#endif
}
