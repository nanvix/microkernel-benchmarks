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

#include <nanvix/sys/noc.h>
#include <nanvix/sys/perf.h>
#include <nanvix/sys/thread.h>
#include <nanvix/sys/semaphore.h>
#include <nanvix/sys/mutex.h>
#include <nanvix/ulib.h>
#include <posix/sys/types.h>
#include <posix/stdint.h>
#include <kbench.h>

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                2  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               2  /**< Increment on Number of Working Threads */
#define OBJSIZE               (1*1024) /**< Maximum Object Size                    */
#define BUFLEN                     16  /**< Buffer Length                          */
#define NOBJECTS                   32  /**< Number of Objects                      */
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
static int NTHREADS;   /**< Number of Working Threads */
/**@}*/

/*============================================================================*
 * Profiling                                                                  *
 *============================================================================*/

/**
 * @brief Name of the benchmark.
 */
#define BENCHMARK_NAME "buffer"

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
 * @param type        Task type.
 * @param nobjs       Number of objects.
 * @param objsize     Object size.
 * @param kland_stats User land execution statistics.
 */
static inline void benchmark_dump_ustats(
	int it,
	const char *type,
	int nobjs,
	size_t objsize,
	uint64_t *uland_stats
)
{
	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s][u] %d %s %d %d %d %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s][u] %d %s %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s][u] %d %s %d %d %d %d",
#endif
		BENCHMARK_NAME,
		it,
		type,
		NTHREADS,
		nobjs,
		objsize,
#if defined(__mppa256__)
		UINT32(uland_stats[0]),
		UINT32(uland_stats[1]),
		UINT32(uland_stats[2]),
		UINT32(uland_stats[3]),
		UINT32(uland_stats[4]),
		UINT32(uland_stats[5]),
		UINT32(uland_stats[6])
#elif defined(__optimsoc__)
		UINT32(uland_stats[0]), /* instruction fetch        */
		UINT32(uland_stats[1]), /* load access              */
		UINT32(uland_stats[2]), /* store access             */
		UINT32(uland_stats[3]), /* instruction fetch stalls */
		UINT32(uland_stats[4]), /* dcache misses            */
		UINT32(uland_stats[5]), /* icache misses            */
		UINT32(uland_stats[6])  /* lsu stalls               */
#else
		UINT32(uland_stats[0])
#endif
	);
}

/**
 * @brief Dump execution statistics.
 *
 * @param it          Benchmark iteration.
 * @param nobjs       Number of objects.
 * @param objsize     Object size.
 * @param kland_stats Kernel land execution statistics.
 */
static inline void benchmark_dump_kstats(
	int it,
	int nobjs,
	size_t objsize,
	uint64_t *kland_stats
)
{
	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s][k] %d %d %d %d %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s][k] %d %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s][k] %d %d %d %d %d",
#endif
		BENCHMARK_NAME,
		it,
		NTHREADS,
		nobjs,
		objsize,
#if defined(__mppa256__)
		UINT32(kland_stats[0]),
		UINT32(kland_stats[1]),
		UINT32(kland_stats[2]),
		UINT32(kland_stats[3]),
		UINT32(kland_stats[4]),
		UINT32(kland_stats[5]),
		UINT32(kland_stats[6])
#elif defined(__optimsoc__)
		UINT32(kland_stats[0]), /* instruction fetch        */
		UINT32(kland_stats[1]), /* load access              */
		UINT32(kland_stats[2]), /* store access             */
		UINT32(kland_stats[3]), /* instruction fetch stalls */
		UINT32(kland_stats[4]), /* dcache misses            */
		UINT32(kland_stats[5]), /* icache misses            */
		UINT32(kland_stats[6])  /* lsu stalls               */
#else
		UINT32(kland_stats[0])
#endif
	);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @brief Current benchmark iteration.
 */
static int iteration = 0;

/**
 * @brief Current performance event.
 */
static int perf = 0;

/**
 * @brief Buffer.
 */
struct buffer
{
	size_t first;
	size_t last;
	struct nanvix_mutex mutex;
	struct nanvix_semaphore full;
	struct nanvix_semaphore empty;
	word_t data[BUFLEN*OBJSIZE/WORD_SIZE];
};

/**
 * @brief Task info.
 */
struct tdata
{
	int n;
	int tnum;
	struct buffer *buf;
	word_t data[OBJSIZE/WORD_SIZE];
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Buffers.
 */
static struct buffer buffers[NTHREADS_MAX/2];

/**
 * @brief Initializes a buffer.
 */
static void buffer_init(struct buffer *buf, int nslots)
{
	buf->first = 0;
	buf->last = 0;
	nanvix_mutex_init(&buf->mutex);
	nanvix_semaphore_init(&buf->full, 0);
	nanvix_semaphore_init(&buf->empty, nslots - 1);
}

/**
 * @brief Puts an object in a buffer.
 *
 * @param buf  Target buffer.
 * @param data Data to place in the buffer.
 */
static inline void buffer_put(struct buffer *buf, const word_t *data)
{
	memcopy(&buf->data[buf->first], data, OBJSIZE/WORD_SIZE);
	buf->first = (buf->first + OBJSIZE/WORD_SIZE)%(BUFLEN*OBJSIZE/WORD_SIZE);
}

/**
 * @brief Gets an object from a buffer.
 *
 * @param buf  Target buffer.
 * @param data Location to store the data.
 */
static inline void buffer_get(struct buffer *buf, word_t *data)
{
	memcopy(data, &buf->data[buf->last], OBJSIZE/WORD_SIZE);
	buf->last = (buf->last + OBJSIZE/WORD_SIZE)%(BUFLEN*OBJSIZE/WORD_SIZE);
}

/**
 * @brief Places objects in a shared buffer.
 */
static void *producer(void *arg)
{
	struct tdata *t = arg;
	struct buffer *buf = t->buf;
	uint64_t uland_stats[BENCHMARK_PERF_EVENTS];

	memfill(t->data, (word_t) -1, OBJSIZE/WORD_SIZE);

	perf_start(0, perf_events[perf]);

		do
		{
			nanvix_semaphore_down(&buf->empty);
				nanvix_mutex_lock(&buf->mutex);

					buffer_put(buf, t->data);

				nanvix_mutex_unlock(&buf->mutex);
			nanvix_semaphore_up(&buf->full);

		} while (--t->n > 0);

	perf_stop(0);
	uland_stats[perf] = perf_read(0);

	if (iteration >= SKIP)
	{
		if (perf == (BENCHMARK_PERF_EVENTS - 1))
		{
			benchmark_dump_ustats(
				iteration - SKIP,
				"p",
				NOBJECTS,
				OBJSIZE,
				uland_stats
			);
		}
	}

	return (NULL);
}

/**
 * @brief Removes objects from a buffer.
 */
static void *consumer(void *arg)
{
	struct tdata *t = arg;
	struct buffer *buf = t->buf;
	uint64_t uland_stats[BENCHMARK_PERF_EVENTS];

	memfill(t->data, 0, OBJSIZE/WORD_SIZE);

	perf_start(0, perf_events[perf]);

		do
		{
			nanvix_semaphore_down(&buf->full);
				nanvix_mutex_lock(&buf->mutex);

					buffer_get(buf, t->data);

				nanvix_mutex_unlock(&buf->mutex);
			nanvix_semaphore_up(&buf->empty);
		} while (--t->n > 0);

	perf_stop(0);
	uland_stats[perf] = perf_read(0);

	if (iteration >= SKIP)
	{
		if (perf == (BENCHMARK_PERF_EVENTS - 1))
		{
			benchmark_dump_ustats(
				iteration - SKIP,
				"c",
				NOBJECTS,
				OBJSIZE,
				uland_stats
			);
		}
	}

	return (NULL);
}

/**
 * @brief Buffer Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 */
static void kernel_buffer(int nthreads)
{
	kthread_t tid[NTHREADS_MAX];
	uint64_t kland_stats[BENCHMARK_PERF_EVENTS];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	/* Spawn threads. */
	for (iteration = 0; iteration < (NITERATIONS + SKIP); iteration++)
	{
		for (perf = 0; perf < BENCHMARK_PERF_EVENTS; perf++)
		{
			kstats(NULL, perf_events[perf]);

			for (int i = 0; i < nthreads; i += 2)
			{
				struct buffer *buf;

				buffer_init(buf = &buffers[i/2], BUFLEN);

				tdata[i].tnum = i;
				tdata[i + 1].tnum = i + 1;
				tdata[i].n = tdata[i + 1].n = NOBJECTS;
				tdata[i].buf = tdata[i + 1].buf = buf;

				kthread_create(&tid[i], producer, &tdata[i]);
				kthread_create(&tid[i + 1], consumer, &tdata[i + 1]);
			}

			/* Wait for threads. */
			for (int i = 0; i < nthreads; i++)
				kthread_join(tid[i], NULL);

			kstats(&kland_stats[perf], perf_events[perf]);
		}

		if (iteration >= SKIP)
		{
			benchmark_dump_kstats(
				iteration - SKIP,
				NOBJECTS,
				OBJSIZE,
				kland_stats
			);
		}
	}
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Producer-Consumer Benchmark
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int __main2(int argc, const char *argv[])
{
	((void) argc);
	((void) argv);

	if (knode_get_num() != PROCESSOR_NODENUM_MASTER)
		return (0);

	uprintf(HLINE);

#ifndef NDEBUG

	kernel_buffer(NTHREADS_MIN);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_buffer(nthreads);

#endif

	uprintf(HLINE);

	return (0);
}
