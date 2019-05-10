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

#ifdef __benchmark_fork_join__

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                1  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               1  /**< Increment on Number of Working Threads */
/**@}*/

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
	uint64_t tfork, tjoin, t0, t1;
	kthread_t tid[NTHREADS_MAX];

	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		t0 = stopwatch_read();

			/* Spawn threads. */
			for (int j = 0; j < nthreads; j++)
				kthread_create(&tid[j], task, NULL);

		t1 = stopwatch_read();

			tfork = stopwatch_diff(t0, t1);

		t0 = stopwatch_read();

			/* Wait for threads. */
			for (int j = 0; j < nthreads; j++)
				kthread_join(tid[j], NULL);

		t1 = stopwatch_read();

		tjoin = stopwatch_diff(t0, t1);

		if (i >= SKIP)
		{
			printf("%s %d %s %d %s %d %s %d",
				"[benchmarks][fork-join]",
				i - SKIP,
				"nthreads",
				nthreads,
				"fork",
				UINT32(tfork),
				"join",
				UINT32(tjoin)
			);
		}
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

#endif /* __benchmark_fork_join__ */
