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
#include <nanvix/ulib.h>
#include <posix/stdint.h>
#include <kbench.h>

/**
 * @brief Horizontal line.
 */
static const char *HLINE =
	"------------------------------------------------------------------------";

/*============================================================================*
 * Profiling                                                                  *
 *============================================================================*/

/**
 * @brief Name of the benchmark.
 */
#define BENCHMARK_NAME "kcall-local"

/**
 * @brief Number of events to profile.
 */
#if defined(__mppa256__)
	#define BENCHMARK_PERF_EVENTS 7
#elif defined(__optimsoc__)
	#define BENCHMARK_PERF_EVENTS 11
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
	MOR1KX_PERF_LOAD_ACCESS,
	MOR1KX_PERF_STORE_ACCESS,
	MOR1KX_PERF_INSTRUCTION_FETCH,
	MOR1KX_PERF_DCACHE_MISSES,
	MOR1KX_PERF_ICACHE_MISSES,
	MOR1KX_PERF_IFETCH_STALLS,
	MOR1KX_PERF_LSU_STALLS,
	MOR1KX_PERF_BRANCH_STALLS,
	MOR1KX_PERF_DTLB_MISSES,
	MOR1KX_PERF_ITLB_MISSES,
	MOR1KX_PERF_DATA_DEP_STALLS
#else
	0
#endif
};

/**
 * @brief Dump execution statistics.
 *
 * @param it    Benchmark iteration.
 * @oaram name  Benchmark name.
 * @param stats Execution statistics.
 */
static void benchmark_dump_stats(int it, const char *name, uint64_t *stats)
{
	uprintf(
#if defined(__mppa256__)
		"[benchmarks][%s] %d %d %d %d %d %d %d %d",
#elif defined(__optimsoc__)
		"[benchmarks][%s] %d %d %d %d %d %d %d %d %d %d %d %d",
#else
		"[benchmarks][%s] %d %d",
#endif
		name,
		it,
#if defined(__mppa256__)
		UINT32(stats[0]),
		UINT32(stats[1]),
		UINT32(stats[2]),
		UINT32(stats[3]),
		UINT32(stats[4]),
		UINT32(stats[5]),
		UINT32(stats[6])
#elif defined(__optimsoc__)
		UINT32(stats[0]), /* instruction fetch        */
		UINT32(stats[1]), /* load access              */
		UINT32(stats[2]), /* store access             */
		UINT32(stats[3]), /* instruction fetch stalls */
		UINT32(stats[4]), /* dcache misses            */
		UINT32(stats[5]), /* icache misses            */
		UINT32(stats[6]), /* lsu stalls               */
		UINT32(stats[7]), /* branch stalls            */
		UINT32(stats[8]), /* dtlb stalls              */
		UINT32(stats[9]), /* itlb stalls              */
		UINT32(stats[10]) /* register stalls          */
#else
		UINT32(stats[0])
#endif
	);
}

/*============================================================================*
 * Benchmark                                                                  *
 *============================================================================*/

/**
 * @brief Local Kernel Call Benchmark
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int __main2(int argc, const char *argv[])
{
	uint64_t stats[BENCHMARK_PERF_EVENTS];

	((void) argc);
	((void) argv);

	/* Skip benchmark. */
	if (cluster_get_num() != PROCESSOR_CLUSTERNUM_MASTER)
		return (0);

	uprintf(HLINE);

	for (int i = 0; i < NITERATIONS + SKIP; i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			perf_start(0, perf_events[j]);

				kcall0(NR_thread_get_id);

			perf_stop(0);
			stats[j] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, BENCHMARK_NAME, stats);
	}

	uprintf(HLINE);

	return (0);
}
