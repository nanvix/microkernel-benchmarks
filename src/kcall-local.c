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
#include "kbench.h"

#ifdef __benchmark_kcall_local__

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                1  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               1  /**< Increment on Number of Working Threads */
/**@}*/

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
 * @brief Issues a local kernel call.
 */
static void *task(void *arg)
{
	struct tdata *t = arg;
	uint64_t t0, t1;

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		perf_start(0, PERF_DCACHE_STALLS);
		perf_start(1, PERF_ICACHE_STALLS);
		t0 = stopwatch_read();

			syscall0(NR_thread_get_id);

		t1 = stopwatch_read();
		perf_stop(1);
		perf_stop(0);

		if (i >= SKIP)
		{
			printf("%s %d %s %d %s %d %s %d %s %d %s %d",
				"[benchmarks][kcall_local]",
				i - SKIP,
				"nthreads",
				NTHREADS,
				"tnum",
				t->tnum,
				"cycles",
				UINT32(stopwatch_diff(t0, t1)),
				"d-stalls",
				UINT32(perf_read(0)),
				"i-stalls",
				UINT32(perf_read(1))
			);
		}
	}

	return (NULL);
}

/**
 * @brief Local Kernel Call Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 */
static void kernel_kcall_local(int nthreads)
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
 * @brief Local Kernel Call Benchmark
 *
 * @param size Transfer size.
 */
void benchmark_kcall_local(void)
{
#ifndef NDEBUG

	kernel_kcall_local(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_kcall_local(nthreads);

#endif
}

#endif /* __benchmark_kcall_local__ */
