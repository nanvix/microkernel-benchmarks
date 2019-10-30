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
#include <nanvix/sys/perf.h>
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
#define BENCHMARK_NAME "kcall-remote"

/**
 * @brief Number of events to profile.
 */
#if defined(__mppa256__)
	#define BENCHMARK_PERF_EVENTS 7
#elif defined(__optimsoc__)
	#define BENCHMARK_PERF_EVENTS 5
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
 * @param it    Benchmark iteration.
 * @oaram name  Benchmark name.
 * @param stats Execution statistics.
 */
static void benchmark_dump_stats(int it, const char *name, uint64_t *stats)
{
	uprintf(
#if (BENCHMARK_PERF_EVENTS >= 7)
		"[benchmarks][%s] %d %d %d %d %d %d %d %d %d",
#elif (BENCHMARK_PERF_EVENTS >= 5)
		"[benchmarks][%s] %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s] %d %d %d",
#endif
		name,
		it,
		NTHREADS,
#if (BENCHMARK_PERF_EVENTS >= 7)
		UINT32(stats[6]),
		UINT32(stats[5]),
#endif
#if (BENCHMARK_PERF_EVENTS >= 5)
		UINT32(stats[4]),
		UINT32(stats[3]),
		UINT32(stats[2]),
		UINT32(stats[1]),
#endif
		UINT32(stats[0])
	);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @brief Thread info.
 */
struct tdata
{
	int tnum;  /**< Thread Number */
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Issues a remote kernel call.
 */
static void *task(void *arg)
{
	struct tdata *t = arg;
	uint64_t stats[BENCHMARK_PERF_EVENTS];

	UNUSED(t);

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			kstats(NULL, perf_events[j]);

				kcall0(NR_SYSCALLS);

			kstats(&stats[j], 0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, BENCHMARK_NAME, stats);
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

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Remote Kernel Call Benchmark
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

	kernel_kcall_remote(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_kcall_remote(nthreads);

#endif

	uprintf(HLINE);

	return (0);
}
