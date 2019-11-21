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
#include <nanvix/sys/signal.h>
#include <nanvix/ulib.h>
#include <posix/stdint.h>
#include <kbench.h>

/**
 * @brief Horizontal line.
 */
static const char *HLINE =
	"------------------------------------------------------------------------";

/*============================================================================*
 * Profiling                                                                  *
 *============================================================================*/

/**
 * @brief Name of the benchmark.
 */
#define BENCHMARK_NAME "upcall"

/**
 * @brief Number of events to profile.
 */
#if defined(__mppa256__)
	#define BENCHMARK_PERF_EVENTS 7
#elif defined(__optimsoc__)
	#define BENCHMARK_PERF_EVENTS 7
#else
	#define BENCHMARK_PERF_EVENTS 1
#endif

/**
 * @briel Benchmark timestamps
 */
static uint64_t perf_value;

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
	MOR1KX_PERF_LSU_HITS,
	MOR1KX_PERF_BRANCH_STALLS,
	MOR1KX_PERF_ICACHE_HITS,
	MOR1KX_PERF_REG_STALLS,
	MOR1KX_PERF_ICACHE_MISSES,
	MOR1KX_PERF_IFETCH_STALLS,
	MOR1KX_PERF_LSU_STALLS,
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
#if defined(__mppa256__)
		"[benchmarks][%s] %d %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s] %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s] %d %d",
#endif
		name,
		it,
#if defined(__mppa256__)
		UINT32(stats[0]),
		UINT32(stats[1]),
		UINT32(stats[2]),
		UINT32(stats[3]),
		UINT32(stats[4]),
		UINT32(stats[5]),
		UINT32(stats[6])
#elif defined(__optimsoc__)
		UINT32(stats[0]),
		UINT32(stats[1]),
		UINT32(stats[2]),
		UINT32(stats[3]),
		UINT32(stats[4]),
		UINT32(stats[5]),
		UINT32(stats[6])
#else
		UINT32(stats[0])
#endif
	);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @brief Page fault handler.
 *
 * @param arg Unused argument.
 */
static void handler(void *arg)
{
	((void) arg);

	/* Stops measurements. */
	perf_stop(0);

	/* Read measurements. */
	perf_value = perf_read(0);
}

/**
 * @brief This functions generates a page fault.
 *
 * @param arg Unused argument.
 */
static void *task(void *arg)
{
	int tmp;
	int perfid;

	/* Gets perf ID. */
	perfid = ((int *) arg)[0];

	/* Starts measurements. */
	perf_start(0, perfid);

	/* Triggers a page fault. */
	tmp = *((int *) NULL);

	/* Unreacheable. */
	UNUSED(tmp);

	return (NULL);
}

/**
 * @brief Upcall Benchmark Kernel
 */
void benchmark_upcall(void)
{
	kthread_t tid;
	struct ksigaction sigact;
	uint64_t upcall_stats[BENCHMARK_PERF_EVENTS];

	/* Sets the page fault handler. */
	sigact.handler = handler;
	ksigctl(SIGPGFAULT, &sigact);

	/* Executes benchmarks. */
	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			/* Spawn a thread. */
			kthread_create(&tid, task, ((void *) &perf_events[j]));

			/* Wait for the thread. */
			kthread_join(tid, NULL);

			/* Saves measurements. */
			upcall_stats[j] = perf_value;
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, BENCHMARK_NAME, upcall_stats);
	}

	/* Unsets the page fault handler. */
	sigact.handler = NULL;
	ksigctl(SIGPGFAULT, &sigact);
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Upcall Benchmark Kernel
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int __main2(int argc, const char *argv[])
{
	((void) argc);
	((void) argv);

	uprintf(HLINE);

	benchmark_upcall();

	uprintf(HLINE);

	return (0);
}
