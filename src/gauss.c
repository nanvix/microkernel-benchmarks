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

#ifdef __benchmark_gauss__

/**
 * @name Math Costants
 */
/**@{*/
#define PI 3.14159265359    /**< Pi                 */
#define E  2.71828182845904 /**< Epsilon            */
#define SD 0.8              /**< Standard Deviation */
/**@}*/

/**
 * @brief Number of events to profile.
 */
#define BENCHMARK_PERF_EVENTS 7

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                      2  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX        (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP                     2  /**< Increment on Number of Working Threads */
#define MASKSIZE                          7  /**< Mask Size                              */
#define IMGSIZE        (56 + (MASKSIZE - 1)) /**< Image Size                             */
/**@}*/

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
 * @name Benchmark Kernel Parameters
 */
/**@{*/
static int NTHREADS; /**< Number of Working Threads */
/**@}*/

/**
 * @brief Task info.
 */
struct tdata
{
	int tnum; /**< Thread Number */
	int i0;   /**< Start Line    */
	int in;   /**< End Line      */
} tdata[NTHREADS_MAX] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Mask.
 */
static double mask[MASKSIZE*MASKSIZE] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Image.
 */
static unsigned char img[IMGSIZE*IMGSIZE] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Output image.
 */
static unsigned char output[IMGSIZE*IMGSIZE] ALIGN(CACHE_LINE_SIZE);

/**
 * @brief Indexes the mask.
 */
#define MASK(i, j) mask[(i)*MASKSIZE + (j)]

/**
 * @brief Indexes the image.
 */
#define IMG(i, j) img[(i)*IMGSIZE + (j)]

/**
 * @brief Indexes the output image.
 */
#define OUTPUT(i, j) output[(i)*IMGSIZE + (j)]

/**
 * @brief Raises a number to a power.
 *
 * @param x Number.
 * @param y Power.
 *
 * @returns x^y.
 */
static double powerd(double x, int y)
{
	double temp;

	if (y == 0)
		return 1;

	temp = powerd(x, y/2);

	if ((y % 2) == 0)
		return (temp*temp);

	if (y > 0)
		return (x*temp*temp);

	return (temp*temp)/x;
}

/**
 * @brief Dump execution statistics.
 *
 * @param it       Benchmark iteration.
 * @param imgsize  Matrix size.
 * @param masksize Mask size.
 * @param stats    Execution statistics.
 */
static inline void benchmark_dump_stats(int it, size_t imgsize, int masksize, uint64_t *stats)
{
	printf("%s %d %d %d %d %d %d %d %d %d %d %d %d",
		"[benchmarks][gauss]",
		it,
		NTHREADS,
		imgsize,
		masksize,
		UINT32(stats[0]),
		UINT32(stats[1]),
		UINT32(stats[2]),
		UINT32(stats[3]),
		UINT32(stats[4]),
		UINT32(stats[5]),
		UINT32(stats[6]),
		UINT32(stats[7])
	);
}

/**
 * @brief Generates the mask.
 */
static inline void generate_mask(void)
{
	int half = MASKSIZE >> 1;;
	double first = 1.0/(2.0*PI*SD*SD);;
	double total = 0.0;

	for (int i = -half; i <= half; i++)
	{
		for (int j = -half; j <= half; j++)
		{
			double sec;

			sec = -((i*i + j*j)/2.0*SD*SD);
			sec = powerd(E, sec);

			MASK(i + half, j + half) = first*sec;
			total += MASK(i + half, j + half);
		}
	}

	for (int i = 0 ; i < MASKSIZE; i++)
	{
		for (int j = 0; j < MASKSIZE; j++)
			MASK(i, j) /= total;
	}
}

/**
 * @brief Applies a gaussian filter to an image.
 *
 * @param i0 Start line.
 * @param in End line.
 */
static inline void gauss_filter(int i0, int in)
{
	int half = MASKSIZE >> 1;

	for (int imgI = i0 + half; imgI < in - half; imgI++)
	{
		for (int imgJ = half; imgJ < IMGSIZE - half; imgJ++)
		{
			double pixel = 0.0;

			for (int maskI = 0; maskI < MASKSIZE; maskI++)
			{
				for (int maskJ = 0; maskJ < MASKSIZE; maskJ++)
				{
					pixel +=
						IMG(imgI + maskI - half, imgJ + maskJ - half)*
						MASK(maskI, maskJ);
				}
			}

			OUTPUT(imgI, imgJ) =
				(pixel > 255) ? 255 : (unsigned char) pixel;
		}
	}
}

/**
 * @brief Multiplies matrices.
 */
static void *task(void *arg)
{
	struct tdata *t = arg;
	int i0 = t->i0;
	int in = t->in;
	uint64_t t0, t1, tmp;
	uint64_t stats[BENCHMARK_PERF_EVENTS + 1];

	stats[0] = UINT64_MAX;

	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		for (int j = 0; j < BENCHMARK_PERF_EVENTS; j++)
		{
			t0 = stopwatch_read();
			perf_start(0, perf_events[j]);

				gauss_filter(i0, in);

			perf_stop(0);
			t1 = stopwatch_read();

			tmp = stopwatch_diff(t0, t1);
			if (tmp < stats[0])
				stats[0] = tmp;
			stats[j + 1] = perf_read(0);
		}

		if (i >= SKIP)
			benchmark_dump_stats(i - SKIP, IMGSIZE, MASKSIZE, stats);
	}

	return (NULL);
}

/**
 * @brief Matrix Multiplication Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 */
static void kernel_gauss(int nthreads)
{
	int nrows;
	kthread_t tid[NTHREADS_MAX];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	nrows = IMGSIZE/nthreads;

	/* Spawn threads. */
	for (int i = 0; i < nthreads; i++)
	{
		/* Initialize thread data structure. */
		tdata[i].i0 = nrows*i;
		tdata[i].in = (i == (nthreads - 1)) ? IMGSIZE : (i + 1)*nrows;
		tdata[i].tnum = i;

		kthread_create(&tid[i], task, &tdata[i]);
	}

	/* Wait for threads. */
	for (int i = 0; i < nthreads; i++)
		kthread_join(tid[i], NULL);
}

/**
 * @brief Matrix Multiplication Benchmark
 */
void benchmark_gauss(void)
{
#ifndef NDEBUG

	kernel_gauss(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_gauss(nthreads);

#endif
}

#endif /* __benchmark_gauss__ */
