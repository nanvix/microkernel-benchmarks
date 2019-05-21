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
#include "kbench.h"

#ifdef __benchmark_upcall__

/**
 * @briel Benchmark timestamps
 */
static uint64_t t0, t1, perf_value;

/**
 * @brief Number of events to profile.
 */
#define BENCHMARK_PERF_EVENTS 7

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
 * @brief Dump execution statistics.
 *
 * @param it    Benchmark iteration.
 * @param stats Execution statistics.
 */
static inline void benchmark_dump_stats(int it, uint64_t *stats)
{
	printf("%s %d %d %d %d %d %d %d %d %d",
		"[benchmarks][upcall]",
		it,
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
 * @brief Page fault handler.
 *
 * @param arg Unused argument.
 */
static void handler(void *arg)
{
	/* Stops measurements. */
	perf_stop(0);

	/* Read measurements. */
	t1 = stopwatch_read();
	perf_value = perf_read(0);

	/* Exit. */
	kthread_exit(arg);
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
	perfid = ((int) arg);

	/* Starts measurements. */
	t0 = stopwatch_read();
	perf_start(0, perfid);

	/* Triggers a page fault. */
	tmp = *((int *) 0xdeadbeef);

	/* Unreacheable. */
	UNUSED(tmp);

	return (NULL);
}

/**
 * @brief Upcall Benchmark
 */
void benchmark_upcall(void)
{
	uint64_t tmp;
	kthread_t tid;
	struct sigaction sigact;
	uint64_t upcall_stats[BENCHMARK_PERF_EVENTS + 1];

	upcall_stats[0] = UINT64_MAX;

	/* Sets the page fault handler. */
	sigact.handler = handler;
	KASSERT(ksigclt(SIGPGFAULT, &sigact) == 0);

	/* Executes benchmarks. */
	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			/* Spawn a thread. */
			kthread_create(&tid, task, ((void *) perf_events[j]));

			/* Wait for the thread. */
			kthread_join(tid, NULL);

			/* Saves measurements. */
			tmp = stopwatch_diff(t0, t1);
			if (tmp < upcall_stats[0])
				upcall_stats[0] = tmp;
			upcall_stats[j + 1] = perf_value;
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, upcall_stats);
	}

	/* Unsets the page fault handler. */
	sigact.handler = NULL;
	KASSERT(ksigclt(SIGPGFAULT, &sigact) == 0);
}

#endif /* __benchmark_upcall__ */
