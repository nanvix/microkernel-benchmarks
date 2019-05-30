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
 * @brief Thread info.
 */
struct tdata
{
	int tnum;  /**< Thread Number */
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

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
static inline void benchmark_dump_stats(int it, uint64_t *stats)
{
	printf("%s %d %d %d %d %d %d %d %d %d %d",
		"[benchmarks][kcall-remote]",
		it,
		NTHREADS,
		UINT32(stats[0]),
		UINT32(stats[1]),
		UINT32(stats[2]),
		UINT32(stats[3]),
		UINT32(stats[4]),
		UINT32(stats[5]),
		UINT32(stats[6]),
		UINT32(stats[7])
	);
}

/**
 * @brief Issues a remote kernel call.
 */
static void *task(void *arg)
{
	struct tdata *t = arg;
	uint64_t t0, t1, tmp;
	uint64_t stats[BENCHMARK_PERF_EVENTS + 1];

	UNUSED(t);

	stats[0] = UINT64_MAX;

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			t0 = stopwatch_read();
			perf_start(0, perf_events[j]);

				syscall0(NR_SYSCALLS);

			perf_stop(0);
			t1 = stopwatch_read();

			tmp = stopwatch_diff(t0, t1);
			if (tmp < stats[0])
				stats[0] = tmp;
			stats[j + 1] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, stats);
	}

	return (NULL);
}

/**
 * @brief Remote Kernel Call Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 */
static void kernel_kcall_remote(int nthreads)
{
	kthread_t tid[NTHREADS_MAX];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	/* Spawn threads. */
	for (int i = 0; i < nthreads; i++)
	{
		/* Initialize thread data structure. */
		tdata[i].tnum = i;

		kthread_create(&tid[i], task, &tdata[i]);
	}

	/* Wait for threads. */
	for (int i = 0; i < nthreads; i++)
		kthread_join(tid[i], NULL);
}

/**
 * @brief Remote Kernel Call Benchmark
 *
 * @param size Transfer size.
 */
void benchmark_kcall_remote(void)
{
#ifndef NDEBUG

	kernel_kcall_remote(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_kcall_remote(nthreads);

#endif
}
