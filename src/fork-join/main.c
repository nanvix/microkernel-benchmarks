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

#include <nanvix/sys/thread.h>
#include <nanvix/ulib.h>
#include <posix/stdint.h>
#include <kbench.h>

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                1  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               1  /**< Increment on Number of Working Threads */
/**@}*/

/**
 * @brief Horizontal line.
 */
static const char *HLINE =
	"------------------------------------------------------------------------";

/**
 * @name Benchmark Kernel Parameters
 */
/**@{*/
static int NTHREADS; /**< Number of Working Threads */
/**@}*/

/*============================================================================*
 * Profiling                                                                  *
 *============================================================================*/

/**
 * @brief Name of the benchmark.
 */
#define BENCHMARK_NAME "fork-join"

/**
 * @brief Number of events to profile.
 */
#if defined(__mppa256__)
	#define BENCHMARK_PERF_EVENTS 7
#elif defined(__optimsoc__)
	#define BENCHMARK_PERF_EVENTS 11
#else
	#define BENCHMARK_PERF_EVENTS 1
#endif

/**
 * Performance events.
 */
static int perf_events[BENCHMARK_PERF_EVENTS] = {
#if defined(__mppa256__)
	PERF_DTLB_STALLS,
	PERF_ITLB_STALLS,
	PERF_REG_STALLS,
	PERF_BRANCH_STALLS,
	PERF_DCACHE_STALLS,
	PERF_ICACHE_STALLS,
	PERF_CYCLES
#elif defined(__optimsoc__)
	PERF_REG_STALLS,
	PERF_BRANCH_STALLS,
	PERF_DCACHE_STALLS,
	PERF_ICACHE_STALLS,
	PERF_CYCLES
#else
	0
#endif
};

/**
 * @brief Dump execution statistics.
 *
 * @param it         Benchmark iteration.
 * @oaram name       Benchmark name.
 * @param fork_stats Fork statistics.
 * @param join_stats Join statistics.
 */
static void benchmark_dump_stats(int it, const char *name, uint64_t *fork_stats, uint64_t *join_stats)
{
	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s] %d %d %s %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s] %d %d %s %d %d %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s] %d %d %s %d %d",
#endif
		name,
		it,
		"f",
		NTHREADS,
#if defined(__mppa256__)
		UINT32(fork_stats[0]),
		UINT32(fork_stats[1]),
		UINT32(fork_stats[2]),
		UINT32(fork_stats[3]),
		UINT32(fork_stats[4]),
		UINT32(fork_stats[5]),
		UINT32(fork_stats[6])
#elif defined(__optimsoc__)
		UINT32(fork_stats[0]), /* instruction fetch        */
		UINT32(fork_stats[1]), /* load access              */
		UINT32(fork_stats[2]), /* store access             */
		UINT32(fork_stats[3]), /* instruction fetch stalls */
		UINT32(fork_stats[4]), /* dcache misses            */
		UINT32(fork_stats[5]), /* icache misses            */
		UINT32(fork_stats[6]), /* lsu stalls               */
		UINT32(fork_stats[7]), /* branch stalls            */
		UINT32(fork_stats[8]), /* dtlb stalls              */
		UINT32(fork_stats[9]), /* itlb stalls              */
		UINT32(fork_stats[10]) /* register stalls          */
#else
		UINT32(fork_stats[0])
#endif
	);

	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s] %d %d %s %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s] %d %d %s %d %d %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s] %d %d %s %d %d",
#endif
		name,
		it,
		"j",
		NTHREADS,
#if defined(__mppa256__)
		UINT32(join_stats[0]),
		UINT32(join_stats[1]),
		UINT32(join_stats[2]),
		UINT32(join_stats[3]),
		UINT32(join_stats[4]),
		UINT32(join_stats[5]),
		UINT32(join_stats[6])
#elif defined(__optimsoc__)
		UINT32(join_stats[0]), /* instruction fetch        */
		UINT32(join_stats[1]), /* load access              */
		UINT32(join_stats[2]), /* store access             */
		UINT32(join_stats[3]), /* instruction fetch stalls */
		UINT32(join_stats[4]), /* dcache misses            */
		UINT32(join_stats[5]), /* icache misses            */
		UINT32(join_stats[6]), /* lsu stalls               */
		UINT32(join_stats[7]), /* branch stalls            */
		UINT32(join_stats[8]), /* dtlb stalls              */
		UINT32(join_stats[9]), /* itlb stalls              */
		UINT32(join_stats[10]) /* register stalls          */
#else
		UINT32(join_stats[0])
#endif
	);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

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
	uint64_t fork_stats[BENCHMARK_PERF_EVENTS];
	uint64_t join_stats[BENCHMARK_PERF_EVENTS];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			perf_start(0, perf_events[j]);

				/* Spawn threads. */
				for (int k = 0; k < nthreads; k++)
					kthread_create(&tid[k], task, NULL);

			perf_stop(0);
			fork_stats[j] = perf_read(0);

			perf_start(0, perf_events[j]);

				/* Wait for threads. */
				for (int k = 0; k < nthreads; k++)
					kthread_join(tid[k], NULL);

			perf_stop(0);
			join_stats[j] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, BENCHMARK_NAME, fork_stats, join_stats);
	}
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Fork-Join Benchmark
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int __main2(int argc, const char *argv[])
{
	((void) argc);
	((void) argv);

	uprintf(HLINE);

#ifndef NDEBUG

	kernel_fork_join(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_fork_join(nthreads);

#endif

	uprintf(HLINE);

	return (0);
}
