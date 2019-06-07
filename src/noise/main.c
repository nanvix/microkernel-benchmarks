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
#define NTHREADS_MIN                1  /**< Minimum Number of Worker Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Worker Threads      */
#define NTHREADS_STEP               1  /**< Increment on Number of Worker Threads */
#define FLOPS                 (10008) /**< Number of Floating Point Operations   */
#define NIOOPS                  (100) /**< Number of Floating Point Operations   */
/**@}*/

/**
 * @name Benchmark Kernel Parameters
 */
/**@{*/
static int NWORKERS;      /**< Number of Worker Threads */
static int NIDLE;         /**< Number of Idle Threads   */
static char *NOISE = "y"; /**< Noise On?                */
/**@}*/

/*============================================================================*
 * Profilling                                                                 *
 *============================================================================*/

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
	static spinlock_t lock = SPINLOCK_UNLOCKED;

	spinlock_lock(&lock);

		printf("%s %d %s %d %d %d %d %d %d %d %d %d\n",
			"[benchmarks][noise]",
			it,
			NOISE,
			NWORKERS,
			NIDLE,
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
 * @brief Task info.
 */
static struct tdata
{
	float scratch;  /**< Scratch Variable  */
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Performs some FPU intensive computation.
 */
static void *task_worker(void *arg)
{
	struct tdata *t = arg;
	register float tmp = t->scratch;
	uint64_t stats[BENCHMARK_PERF_EVENTS];

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			perf_start(0, perf_events[j]);

				for (int k = 0; k < FLOPS; k += 9)
				{
					register float k1 = k*1.1;
					register float k2 = k*2.1;
					register float k3 = k*3.1;
					register float k4 = k*4.1;

					tmp += k1 + k2 + k3 + k4;
				}

			perf_stop(0);
			stats[j] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, stats);
	}

	/* Avoid compiler optimizations. */
	t->scratch = tmp;

	return (NULL);
}

/**
 * @brief Issues some remote kernel calls.
 */
static void *task_idle(void *arg)
{
	UNUSED(arg);

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			for (int k = 0; k < NIOOPS; k++)
				syscall0(NR_SYSCALLS);
		}
	}

	return (NULL);
}

/*============================================================================*
 * Kernel Noise Benchmark                                                     *
 *============================================================================*/

/**
 * @brief Kernel Noise Benchmark Kernel
 *
 * @param nworkers Number of worker threads.
 * @param nidle    Number of idle threads.
 */
static void benchmark_noise(int nworkers, int nidle)
{
	kthread_t tid_workers[NTHREADS_MAX];
	kthread_t tid_idle[NTHREADS_MAX];

	/* Save kernel parameters. */
	NWORKERS = nworkers;
	NIDLE = nidle;

	/*
	 * Spawn idle threads first,
	 * so that we have a noisy system.
	 */
	for (int i = 0; i < nidle; i++)
		kthread_create(&tid_idle[i], task_idle, NULL);

	/* Spawn worker threads. */
	for (int i = 0; i < nworkers; i++)
	{
		tdata[i].scratch = 0.0;
		kthread_create(&tid_workers[i], task_worker, &tdata[i]);
	}

	/* Wait for threads. */
	for (int i = 0; i < nworkers; i++)
		kthread_join(tid_workers[i], NULL);
	for (int i = 0; i < nidle; i++)
		kthread_join(tid_idle[i], NULL);
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Kernel Noise Benchmark
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

	benchmark_noise(NTHREADS_MAX/2, NTHREADS_MAX/2);

#else

	/* With noise. */
	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		benchmark_noise(nthreads, NTHREADS_MAX - nthreads);

	/* No noise. */
	NOISE = "n";
	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		benchmark_noise(nthreads, 0);

#endif

	printf(HLINE);

	return (0);
}
