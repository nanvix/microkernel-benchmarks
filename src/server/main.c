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

#include <nanvix/sys/perf.h>
#include <nanvix/sys/thread.h>
#include <nanvix/sys/mutex.h>
#include <nanvix/sys/semaphore.h>
#include <nanvix/ulib.h>
#include <posix/stdint.h>
#include <kbench.h>

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                1  /**< Minimum Number of Worker Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Worker Threads      */
#define NTHREADS_STEP               1  /**< Increment on Number of Worker Threads */
#define NREQUESTS                1000  /**< Number of Requests                    */
#define FLOPS                   10008  /**< Number of Floating Point Operations   */
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
static int NWORKERS; /**< Number of Worker Threads */
/**@}*/

/*============================================================================*
 * Profilling                                                                 *
 *============================================================================*/

/**
 * @brief Name of the benchmark.
 */
#define BENCHMARK_NAME "server"

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
 * @param it          Benchmark iteration.
 * @oaram name        Benchmark name.
 * @param uland_stats User land execution statistics.
 * @param kland_stats Kernel execution statistics.
 */
static void benchmark_dump_stats(
	int it,
	const char *name,
	uint64_t *uland_stats,
	uint64_t *kland_stats
)
{
	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s][u] %d %d %d %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s][u] %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s][u] %d %d %d %d",
#endif
		name,
		it,
		NWORKERS,
		NREQUESTS,
#if defined(__mppa256__)
		UINT32(uland_stats[0]),
		UINT32(uland_stats[1]),
		UINT32(uland_stats[2]),
		UINT32(uland_stats[3]),
		UINT32(uland_stats[4]),
		UINT32(uland_stats[5]),
		UINT32(uland_stats[6])
#elif defined(__optimsoc__)
		UINT32(uland_stats[0]),
		UINT32(uland_stats[1]),
		UINT32(uland_stats[2]),
		UINT32(uland_stats[3]),
		UINT32(uland_stats[4]),
		UINT32(uland_stats[5]),
		UINT32(uland_stats[6])
#else
		UINT32(uland_stats[0])
#endif
	);

	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s][k] %d %d %d %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s][k] %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s][k] %d %d %d %d",
#endif
		name,
		it,
		NWORKERS,
		NREQUESTS,
#if defined(__mppa256__)
		UINT32(kland_stats[0]),
		UINT32(kland_stats[1]),
		UINT32(kland_stats[2]),
		UINT32(kland_stats[3]),
		UINT32(kland_stats[4]),
		UINT32(kland_stats[5]),
		UINT32(kland_stats[6])
#elif defined(__optimsoc__)
		UINT32(kland_stats[0]),
		UINT32(kland_stats[1]),
		UINT32(kland_stats[2]),
		UINT32(kland_stats[3]),
		UINT32(kland_stats[4]),
		UINT32(kland_stats[5]),
		UINT32(kland_stats[6])
#else
		UINT32(kland_stats[0])
#endif
	);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @brief Server information.
 */
struct
{
	kthread_t tids[NTHREADS_MAX];      /**< IDs of Worker Threads         */
	int nworkers;                      /**< Number of Worker Threads      */
	struct nanvix_mutex lock;          /**< Server Lock                   */
	struct nanvix_semaphore workers;   /**< Number of Idle Worker Threads */
} sinfo;

/**
 * @brief Worker threads.
 */
struct winfo
{
	int idle;                       /**< Idle Worker?      */
	int request;                    /**< Current Request   */
	float scratch;                  /**< scratch variable. */
	struct nanvix_semaphore wakeup; /**< Wake Up Worker?   */
} workers[NTHREADS_MAX];

/**
 * @brief Performs CPU intensive computation
 */
static void do_work(float *scratch)
{
	register float tmp = *scratch;

	for (int k = 0; k < FLOPS; k += 9)
	{
		register float k1 = k*1.1;
		register float k2 = k*2.1;
		register float k3 = k*3.1;
		register float k4 = k*4.1;

		tmp += k1 + k2 + k3 + k4;
	}

	/* Avoid compiler optimizations. */
	*scratch = tmp;
}
/**
 * @brief Worker thread.
 */
static void *worker(void *arg)
{
	struct winfo *t = arg;

	while (1)
	{
		nanvix_semaphore_down(&t->wakeup);

		/* Shutdown signal. */
		if (t->request == -1)
			break;

		do_work(&t->scratch);

		nanvix_mutex_lock(&sinfo.lock);
			t->idle = 1;
		nanvix_mutex_unlock(&sinfo.lock);
		nanvix_semaphore_up(&sinfo.workers);
	}

	return (NULL);
}

/**
 * @brief Starts up the sinfo.
 *
 * @param nworkers  Number of worker threads.
 */
static void server_startup(int nworkers)
{
	/* Initialize sinfo. */
	sinfo.nworkers = nworkers;
	nanvix_mutex_init(&sinfo.lock);
	nanvix_semaphore_init(&sinfo.workers, nworkers);

	/* Spawn worker threads. */
	for (int i = 0; i < nworkers; i++)
	{
		workers[i].idle = 1;
		nanvix_semaphore_init(&workers[i].wakeup, 0);
		kthread_create(&sinfo.tids[i], worker, &workers[i]);
	}
}

/**
 * @brief Shuts down the sinfo.
 */
static void server_shutdown(void)
{
again:

	nanvix_mutex_lock(&sinfo.lock);

		/* Broadcast shutdown signal. */
		for (int i = 0; i < sinfo.nworkers; i++)
		{
			if (!workers[i].idle)
			{
				nanvix_mutex_unlock(&sinfo.lock);
				goto again;
			}

			/* Shutdown signal. */
			workers[i].request = -1;
			nanvix_semaphore_up(&workers[i].wakeup);
		}

	nanvix_mutex_unlock(&sinfo.lock);

	/* Join worker threads. */
	for (int i = 0; i < sinfo.nworkers; i++)
		kthread_join(sinfo.tids[i], NULL);

}

/**
 * @brief A simple multi-thread server.
 *
 * @param nworkers  Number of worker threads.
 * @param nrequests Number of requests.
 */
static void server(int nworkers, int nrequests)
{
	uint64_t uland_stats[BENCHMARK_PERF_EVENTS];
	uint64_t kland_stats[BENCHMARK_PERF_EVENTS];

	server_startup(nworkers);

		for (int k = 0; k < NITERATIONS + SKIP; k++)
		{
			for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
			{
				int n = 0;

				perf_start(0, perf_events[j]);
				kstats(NULL, perf_events[j]);

				do
				{
					nanvix_semaphore_down(&sinfo.workers);
					nanvix_mutex_lock(&sinfo.lock);

					/* Dispatch request to an idle worker thread. */
					for (int i = 0; i < nworkers; i++)
					{
						if (workers[i].idle)
						{
							workers[i].request = n++;
							workers[i].idle = 0;
							nanvix_semaphore_up(&workers[i].wakeup);
							break;
						}
					}

					nanvix_mutex_unlock(&sinfo.lock);
				} while (n < nrequests);

				kstats(&kland_stats[j], 0);
				perf_stop(0);
				uland_stats[j] = perf_read(0);
			}

			if (k >= SKIP)
			{
				benchmark_dump_stats(
					k - SKIP,
					BENCHMARK_NAME,
					uland_stats,
					kland_stats
				);
			}
		}

	server_shutdown();
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Server Benchmark
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

	server(NWORKERS = NTHREADS_MAX, NREQUESTS);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		server(NWORKERS = nthreads, NREQUESTS);

#endif

	uprintf(HLINE);

	return (0);
}
