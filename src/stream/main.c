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

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                2  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               4  /**< Increment on Number of Working Threads */
#define OBJSIZE_MIN           (1*1024) /**< Minimum Object Size                    */
#define OBJSIZE_MAX           (8*1024) /**< Maximum Object Size                    */
#define OBJSIZE_STEP          (1*1024) /**< Object Size                            */
/**@}*/

/**
 * @name Benchmark Kernel Parameters
 */
/**@{*/
static int NTHREADS;   /**< Number of Working Threads */
static size_t OBJSIZE; /**< Object Size               */
/**@}*/

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
 * @param it      Benchmark iteration.
 * @param objsize Object size.
 * @param stats   Execution statistics.
 */
static inline void benchmark_dump_stats(int it, size_t objsize, uint64_t *stats)
{
	static spinlock_t lock = SPINLOCK_UNLOCKED;

	spinlock_lock(&lock);

		printf("%s %d %d %d %d %d %d %d %d %d %d\n",
			"[benchmarks][stream]",
			it,
			NTHREADS,
			objsize,
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
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @brief Thread info.
 */
struct tdata
{
	int tnum;  /**< Thread Number */
	size_t start; /**< Start Byte    */
	size_t end;   /**< End Byte      */
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Buffers.
 */
/**@{*/
static word_t obj1[OBJSIZE_MAX/WORD_SIZE] ALIGN(CACHE_LINE_SIZE);
static word_t obj2[OBJSIZE_MAX/WORD_SIZE] ALIGN(CACHE_LINE_SIZE);
/**@}*/

/**
 * @brief Move Bytes in Memory
 */
static void *task(void *arg)
{
	struct tdata *t = arg;
	int start = t->start;
	int end = t->end;
	uint64_t stats[BENCHMARK_PERF_EVENTS];

	/* Warm up. */
	memfill(&obj1[start], (word_t) - 1, end - start);
	memfill(&obj2[start], 0, end - start);

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			perf_start(0, perf_events[j]);

				memcopy(&obj1[start], &obj2[start], end - start);

			perf_stop(0);
			stats[j] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, OBJSIZE, stats);
	}

	return (NULL);
}

/**
 * @brief Memory Move Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 * @param objsize  Object size.
 */
static void kernel_memmove(int nthreads, size_t objsize)
{
	size_t nbytes;
	kthread_t tid[NTHREADS_MAX];

	/* Save kernel parameters. */
	NTHREADS = nthreads;
	OBJSIZE = objsize;

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

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Memory Move Benchmark
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int main(int argc, const char *argv[])
{
	((void) argc);
	((void) argv);

	printf(HLINE);

#ifndef NDEBUG

	kernel_memmove(1, OBJSIZE_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
	{
		for (size_t objsize = OBJSIZE_MIN; objsize <= OBJSIZE_MAX; objsize += OBJSIZE_STEP)
			kernel_memmove(nthreads, objsize);
	}

#endif

	printf(HLINE);

	return (0);
}
