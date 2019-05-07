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

#ifdef __benchmark_memmove__

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                2  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               2  /**< Increment on Number of Working Threads */
#define OBJSIZE              (64*1024) /**< Object Size                            */
/**@}*/

/**
 * @name Benchmark Kernel Parameters
 */
/**@{*/
static int NTHREADS; /**< Number of Working Threads */
/**@}*/

/**
 * @brief Thread info.
 */
struct tdata
{
	int tnum;  /**< Thread Number */
	int start; /**< Start Byte    */
	int end;   /**< End Byte      */
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Buffers.
 */
/**@{*/
static word_t obj1[OBJSIZE/WORD_SIZE] ALIGN(CACHE_LINE_SIZE);
static word_t obj2[OBJSIZE/WORD_SIZE] ALIGN(CACHE_LINE_SIZE);
/**@}*/

/**
 * @brief Move Bytes in Memory
 */
static void *task(void *arg)
{
	struct tdata *t = arg;
	int start = t->start;
	int end = t->end;
	uint64_t t0, t1;

	/* Warm up. */
	memfill(&obj1[start], (word_t) - 1, end - start);
	memfill(&obj2[start], 0, end - start);

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		perf_start(0, PERF_DCACHE_STALLS);
		perf_start(1, PERF_ICACHE_STALLS);
		t0 = stopwatch_read();

			memcopy(&obj1[start], &obj2[start], end - start);

			/*
			 * TODO: flush data cache
			 */

		t1 = stopwatch_read();
		perf_stop(1);
		perf_stop(0);

		if (i >= SKIP)
		{
			printf("%s %d %s %d %s %d %s %d %s %d %s %d %s %d",
				"[benchmarks][memmove]",
				i - SKIP,
				"nthreads",
				NTHREADS,
				"tnum",
				t->tnum,
				"objsize",
				(end - start)*WORD_SIZE,
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
 * @brief Memory Move Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 */
static void kernel_memmove(int nthreads)
{
	int nbytes;
	kthread_t tid[NTHREADS_MAX];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	nbytes = (OBJSIZE/WORD_SIZE)/nthreads;

	/* Spawn threads. */
	for (int i = 0; i < nthreads; i++)
	{
		/* Initialize thread data structure. */
		tdata[i].start = nbytes*i;
		tdata[i].end = (i == (nthreads - 1)) ? (OBJSIZE/WORD_SIZE) : (i + 1)*nbytes;
		tdata[i].tnum = i;

		kthread_create(&tid[i], task, &tdata[i]);
	}

	/* Wait for threads. */
	for (int i = 0; i < nthreads; i++)
		kthread_join(tid[i], NULL);
}

/**
 * @brief Memory Move Benchmark
 */
void benchmark_memmove(void)
{
#ifndef NDEBUG

	kernel_memmove(1);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_memmove(nthreads);

#endif
}

#endif /* __benchmark_memmove__ */
