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
#define NREQUESTS                1000  /**< Number of Requests                    */
#define FLOPS                   10008  /**< Number of Floating Point Operations   */
/**@}*/

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
 * @brief Number of events to profile.
 */
#define BENCHMARK_PERF_EVENTS 1

/**
 * Performance events.
 */
static int perf_events[BENCHMARK_PERF_EVENTS] = {
	PERF_CYCLES
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

		printf("%s %d %d %d %d\n",
			"[benchmarks][server]",
			it,
			NWORKERS,
			NREQUESTS,
			UINT32(stats[0])
		);

	spinlock_unlock(&lock);
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
	uint64_t stats;

	server_startup(nworkers);

		for (int k = 0; k < NITERATIONS + SKIP; k++)
		{
			int n = 0;

			perf_start(0, perf_events[0]);

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

			perf_stop(0);
			stats = perf_read(0);

			if (k >= SKIP)
				benchmark_dump_stats(k - SKIP, &stats);
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
int main(int argc, const char *argv[])
{
	((void) argc);
	((void) argv);

	printf(HLINE);

#ifndef NDEBUG

	server(NWORKERS = NTHREADS_MAX, NREQUESTS);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		server(NWORKERS = nthreads, NREQUESTS);

#endif

	printf(HLINE);

	return (0);
}
