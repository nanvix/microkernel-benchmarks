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
#include <ulibc/limits.h>
#include <nanvix.h>
#include <stdint.h>
#include <kbench.h>

/**
 * @brief Seed for pseudo-random number generator.
 */
#define SEED 12345

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                2  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               2  /**< Increment on Number of Working Threads */
#define NTOWNS                      8  /**< Number of Towns.                       */
#define NPARTITIONS                20  /**< Partitions Per Cluster                 */
/**@}*/

/**
 * @name Other Parameters
 */
/**@{*/
#define MAX_GRID_X		                                   100   /**< Maximum of Lines on Grid            */
#define MAX_GRID_Y		                                   100   /**< Maximum of Columns on Grid          */
#define INITIAL_JOB_DIST                                    50.0 /**< Initial Job Distribution Percentage */
#define MIN_JOBS_PER_THREAD                                 20   /**< Minimum Jobs Per Thread             */
#define MAX_JOBS_PER_THREAD                                150   /**< Maximum Jobs per Thread             */
#define MAX_JOBS_PER_QUEUE  (MAX_JOBS_PER_THREAD * NTHREADS_MAX) /**< Maximum of Jobs Per Thread          */
/**@}*/

/**
 * @name Current Benchmark Parameters
 */
/**@{*/
static int NTHREADS;    /**< Number of Working Threads. */
/**@}*/

/**
 * @name Benchmark Kernel Variables
 */
/**@{*/
static int max_hops;              /**< Maximum number of hops.        */
static int next_partition_id;     /**< Next partition interval ID.    */
static int processed_partitions;  /**< Amount of intervals processed. */
static int min_distance;          /**< Minimum distance found.        */
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
 * @brief Current performance event being monitored.
 */
static int perf;

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
 * @brief Execution statistics.
 */
static uint64_t stats[NTHREADS_MAX][BENCHMARK_PERF_EVENTS] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Dump execution statistics.
 *
 * @param it      Benchmark iteration.
 * @param ntowsn  Number of towns.
 * @param stats   Execution statistics.
 */
static void benchmark_dump_stats(int it, int ntowns, uint64_t *st)
{
	static spinlock_t lock = SPINLOCK_UNLOCKED;

	spinlock_lock(&lock);

#ifdef NDEBUG

	printf("%s %d %d %d %d %d %d %d %d %d %d %d\n",
		"[benchmarks][tsp]",
		it,
		NTHREADS,
		ntowns,
		UINT32(st[0]),
		UINT32(st[1]),
		UINT32(st[2]),
		UINT32(st[3]),
		UINT32(st[4]),
		UINT32(st[5]),
		UINT32(st[6])
	);

#else

	UNUSED(it);

	printf("%s nthreads=%d    ntowns=%d    min_distance=%d    time=%d us\n",
		"[benchmarks][tsp]",
		NTHREADS,
		ntowns,
		min_distance,
		UINT32(st[0])/(TARGET_FREQUENCY)
	);

#endif

	spinlock_unlock(&lock);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @name Concurrency control
 */
/**@{*/
static struct nanvix_fmutex main_lock; /**< Lock for Kernel Variables.           */
static int waiting_threads = 0;       /**< Number of threads waiting the queue. */
/**@}*/

/**
 * @brief Random number generator state.
 */
struct rng_state
{
	unsigned w;
	unsigned z;
};

/**
 * Initializes the pseudo-random number generator.
 */
static void rng_initialize(struct rng_state * state)
{
	unsigned n1, n2;

	n1 = (SEED * 104623) % 4294967296;
	n2 = (SEED * 48947)  % 4294967296;

	state->w = (n1) ? n1 : 521288629;
	state->z = (n2) ? n2 : 362436069;
}

/**
 * Returns the next pseudo-random number.
 */
static unsigned rng_next(struct rng_state * state)
{
	state->z = 36969 * (state->z & 65535) + (state->z >> 16);
	state->w = 18000 * (state->w & 65535) + (state->w >> 16);

	return ((state->z << 16) + state->w);
}

/**
 * Computes n^0.5.
 */
static double squared(double n)
{
	double s;

	if (n <= 0)
		return (0);

	s = n;

	while ((s - n/s) > 0.001)
		s = (s + n/s)/2;

	return (s);
}

struct partition_interval
{
	int start;
	int end;
};

static struct distance_matrix
{
	int to_city;
	int dist;
} distance[NTOWNS][NTOWNS];

static struct jobs_queue
{
	int begin;
	int end;
	int max_size;

	struct nanvix_semaphore semaphore;
	struct nanvix_fmutex lock;

	enum queue_status
	{
		CLOSED_QUEUE = 0,
		WAIT_QUEUE   = 1,
		EMPTY_QUEUE  = 2
	} status;

	struct job
	{
		int lenght;
		int path[NTOWNS];
	} jobs[MAX_JOBS_PER_QUEUE];
} queue;

static void reset_queue(void)
{
	queue.begin = 0;
	queue.end   = 0;
}

static void init_queue(int queue_size)
{
	queue.begin    = 0;
	queue.end      = 0;
	queue.status   = EMPTY_QUEUE;
	queue.max_size = queue_size;

	nanvix_semaphore_init(&queue.semaphore, 0);
	nanvix_fmutex_init(&queue.lock);

	kmemset(&queue.jobs[0], 0, sizeof(struct job) * queue_size);

	waiting_threads = 0;
}

static void close_queue(void)
{
	queue.status = CLOSED_QUEUE;

	nanvix_fmutex_lock(&main_lock);

		for (int i = 0; i < waiting_threads; i++)
			nanvix_semaphore_up(&queue.semaphore);

	nanvix_fmutex_unlock(&main_lock);
}

static void enqueue(struct job *job)
{
	nanvix_fmutex_lock(&queue.lock);

		kmemcpy(&queue.jobs[queue.end], job, sizeof(struct job));

		queue.end++;

		nanvix_semaphore_up(&queue.semaphore);

	nanvix_fmutex_unlock(&queue.lock);
}

static int repopulate_queue(void);

static int dequeue(struct job * job)
{
	int index;
	int jobs_added;

	nanvix_fmutex_lock(&queue.lock);

		while (queue.begin == queue.end)
		{
			switch (queue.status)
			{
				case CLOSED_QUEUE:
					nanvix_fmutex_unlock(&queue.lock);
					return (0);

				case WAIT_QUEUE:
					nanvix_fmutex_unlock(&queue.lock);

						nanvix_fmutex_lock(&main_lock);

							waiting_threads++;

						nanvix_fmutex_unlock(&main_lock);

						nanvix_semaphore_down(&queue.semaphore);

						nanvix_fmutex_lock(&main_lock);

							waiting_threads++;

						nanvix_fmutex_unlock(&main_lock);

					nanvix_fmutex_lock(&queue.lock);
					break;

				case EMPTY_QUEUE:
					queue.status = WAIT_QUEUE;
					reset_queue();

					nanvix_fmutex_unlock(&queue.lock);

						jobs_added = repopulate_queue();

					nanvix_fmutex_lock(&queue.lock);

					if (jobs_added)
						queue.status = EMPTY_QUEUE;
					else
						close_queue();

					break;

				/* Unreachable. */
				default:
					UNREACHABLE();
					break;
			}
		}

		index = queue.begin++;

		kmemcpy(job, &queue.jobs[index], sizeof(struct job));

	nanvix_fmutex_unlock(&queue.lock);

	return (1);
}

static inline int present(int city, int hops, int *path)
{
	for (int i = 0; i < hops; i++)
	{
		if (path[i] == city)
			return (1);
	}

	return (0);
}

static void get_next_partition(struct partition_interval * partition)
{
	int alfa;
	int block_size2;
	int block_size;

	alfa = 1;
	block_size = NPARTITIONS / (1.0 / (INITIAL_JOB_DIST / 100.0));

	if (processed_partitions != 0)
	{
		block_size2 = (NPARTITIONS - next_partition_id) / alfa;
		block_size = (block_size2 < block_size) ? block_size2 : block_size;
	}

	if (block_size < 1)
		block_size = 1;

	partition->start = partition->end = -1;

	if (next_partition_id < NPARTITIONS)
		partition->start = next_partition_id;

	if (next_partition_id + block_size - 1 < NPARTITIONS)
		partition->end = next_partition_id + block_size - 1;
	else
		partition->end = partition->start + NPARTITIONS - partition->start;

	next_partition_id += block_size;
}

static void distributor(
	struct partition_interval *partition,
	int hops,
	int lenght,
	int *path,
	int *jobs_count
)
{
	int me;
	int city;
	int dist;
	int job_id;
	struct job new_job;

	/* End recursion */
	if (hops == max_hops)
	{
		job_id = (*jobs_count) % NPARTITIONS;

		if ((job_id >= partition->start) && (job_id <= partition->end))
		{
			new_job.lenght = lenght;

			for (int i = 0; i < hops; i++)
				new_job.path[i] = path[i];

			enqueue(&new_job);
		}

		(*jobs_count)++;
	}

	/* Go down */
	else
	{
		me = path[hops - 1];

		for (int i = 0; i < NTOWNS; i++)
		{
			city = distance[me][i].to_city;

			if (!present(city, hops, path))
			{
				path[hops] = city;
				dist = distance[me][i].dist;

				distributor(partition, (hops + 1), (lenght + dist), path, jobs_count);
			}
		}
	}
}

static int repopulate_queue(void)
{
	int jobs_count;
	int path[NTOWNS];
	struct partition_interval partition;

	get_next_partition(&partition);

	if (partition.start < 0)
		return (0);

	processed_partitions += (partition.end - partition.start + 1);

	/* Generate jobs. */

	path[0] = 0;
	jobs_count = 0;

	distributor(&partition, 1, 0, path, &jobs_count);

	return (1);
}

static int get_shortest_lenght(void)
{
	int dist;

	nanvix_fmutex_lock(&main_lock);

		dist = min_distance;

	nanvix_fmutex_unlock(&main_lock);

	return (dist);
}

static int update_minimum_distance(int new_distance)
{
	int updated;
	int old_distance;

	updated = 0;

	nanvix_fmutex_lock(&main_lock);

		old_distance = min_distance;

		if (new_distance < old_distance)
		{
			min_distance = new_distance;

			updated = 1;
		}

	nanvix_fmutex_unlock(&main_lock);

	return (updated);
}

static void execute_tsp(int hops, int lenght, int *path)
{
	int me;
	int city;
	int dist;

	if (lenght >= get_shortest_lenght())
		return;

	/* End recursion. */
	if (hops == NTOWNS)
		update_minimum_distance(lenght);

	/* Go down. */
	else
	{
		me = path[hops - 1];

		for (int i = 0; i < NTOWNS; i++)
		{
			city = distance[me][i].to_city;

			if (!present(city, hops, path))
			{
				path[hops] = city;
				dist = distance[me][i].dist;
				execute_tsp((hops + 1), (lenght + dist), path);
			}
		}
	}
}

/**
 * @brief Gets TSP jobs.
 */
static void *worker(void *arg)
{
	int found;
	struct job job;
	int tid = *((int *) arg);

	perf_start(0, perf_events[perf]);

		while (1)
		{
			found = dequeue(&job);

			if (!found)
				break;

			execute_tsp(max_hops, job.lenght, job.path);
		}

	perf_stop(0);
	stats[tid][perf] = perf_read(0);

	return (NULL);
}

static void init_distance(void)
{
	int tmp;
	int city;
	int dx, dy;
	int x[NTOWNS], y[NTOWNS], tempdist[NTOWNS];
	struct rng_state rand_state;

	city = 0;

	rng_initialize(&rand_state);

	for (int i = 0; i < NTOWNS; i++)
	{
		x[i] = rng_next(&rand_state) % MAX_GRID_X;
		y[i] = rng_next(&rand_state) % MAX_GRID_Y;
	}

	for (int i = 0; i < NTOWNS; i++)
	{
		for (int j = 0; j < NTOWNS; j++)
		{
			dx = x[i] - x[j];
			dy = y[i] - y[j];
			tempdist[j] = ((int) squared((double) ((dx * dx) + (dy * dy))));
		}

		for (int j = 0; j < NTOWNS; j++)
		{
			tmp = INT_MAX;

			for (int k = 0; k < NTOWNS; k++)
			{
				if (tempdist[k] < tmp)
				{
					tmp = tempdist[k];
					city = k;
				}
			}

			tempdist[city] = INT_MAX;
			distance[i][j].to_city = city;
			distance[i][j].dist    = tmp;
		}
	}
}

static int init_max_hops(void)
{
	int total;
	int new_total;

	total = 1;
	new_total = 1;
	max_hops = 0;

	while ((new_total < (MIN_JOBS_PER_THREAD * NTHREADS)) && (max_hops < (NTOWNS - 1)))
	{
		max_hops++;
		total = new_total;
		new_total *= (NTOWNS - max_hops);
	}

	return (total);
}

static void init_tsp(int nthreads, int ntowns)
{
	int qsize;

	((void) ntowns);

	nanvix_fmutex_init(&main_lock);

	NTHREADS     = nthreads;
	min_distance = INT_MAX;
	next_partition_id = 0;
	processed_partitions = 0;

	init_distance();

	qsize = init_max_hops();
	qsize = qsize + qsize;

	init_queue(qsize);
}

static void finish_tsp(void)
{
}

/**
 * @brief Travelling Salesman Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 * @param ntowns   Number of towns.
 */
static void kernel_tsp(int nthreads, int ntowns)
{
	int tnum[NTHREADS_MAX];

	for (int k = 0; k < (NITERATIONS + SKIP); k++)
	{
		kthread_t tid[NTHREADS_MAX];

		for (perf = 0; perf < BENCHMARK_PERF_EVENTS; perf++)
		{
			/* Save kernel parameters. */
			init_tsp(nthreads, ntowns);

			/* Spawn threads. */
			for (int i = 0; i < nthreads; i++)
			{
				tnum[i] = i;
				kthread_create(&tid[i], worker, &tnum[i]);
			}

			/* Wait for threads. */
			for (int i = 0; i < nthreads; i++)
				kthread_join(tid[i], NULL);

			/* House keeping. */
			finish_tsp();
		}

		/* Dump statistics. */
		if (k >= SKIP)
		{
			for (int i = 0; i < nthreads; i++)
				benchmark_dump_stats(k - SKIP, ntowns, &stats[i][0]);
		}
	}
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Travelling Salesman Benchmark
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

	kernel_tsp(NTHREADS_MAX, NTOWNS);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_tsp(nthreads, NTOWNS);

#endif

	printf(HLINE);

	return (0);
}
