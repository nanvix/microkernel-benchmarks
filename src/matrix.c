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

#ifdef __benchmark_matrix__

/**
 * @name Benchmark Parameters
 */
/**@{*/
#define NTHREADS_MIN                2  /**< Minimum Number of Working Threads      */
#define NTHREADS_MAX  (THREAD_MAX - 1) /**< Maximum Number of Working Threads      */
#define NTHREADS_STEP               2  /**< Increment on Number of Working Threads */
#define MATSIZE                    84  /**< Matrix Size                            */
/**@}*/

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
 * @brief Matrices.
 */
/**@{*/
static float a[MATSIZE*MATSIZE] ALIGN(CACHE_LINE_SIZE);
static float b[MATSIZE*MATSIZE] ALIGN(CACHE_LINE_SIZE);
static float ret[MATSIZE*MATSIZE] ALIGN(CACHE_LINE_SIZE);
/**@}*/

/**
 * @brief Initializes a chunk of the matrix.
 *
 * @param i0 Start line.
 * @param in End line.
 */
static inline void matrix_init(int i0, int in)
{
	for (int i = i0; i < in; i++)
	{
		for (int j = 0; j < MATSIZE; j++)
		{
			a[i*MATSIZE + j] = 1.0;
			b[i*MATSIZE + j] = 1.0;
		}
	}
}

/**
 * @brief Multiples a chunk of the matrices.
 *
 * @param i0 Start line.
 * @param in End line.
 */
static inline void matrix_mult(int i0, int in)
{
	for (int i = i0; i < in; ++i)
	{
		int ii = i*MATSIZE;

		for (int j = 0; j < MATSIZE; ++j)
		{
			float c = 0;

			for (int k = 0; k < MATSIZE; k += 4)
			{
				c += a[ii + k]*b[k*MATSIZE + j];
				c += a[ii + k + 1]*b[k*MATSIZE + j + 1];
				c += a[ii + k + 2]*b[k*MATSIZE + j + 2];
				c += a[ii + k + 3]*b[k*MATSIZE + j + 3];
			}

			ret[ii + j] = c;
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
	uint64_t t0, t1;

	for (int i = 0; i < (NITERATIONS + SKIP); i++)
	{
		matrix_init(i0, in);

		perf_start(0, PERF_DCACHE_STALLS);
		perf_start(1, PERF_ICACHE_STALLS);
		t0 = stopwatch_read();

			matrix_mult(i0, in);

		t1 = stopwatch_read();
		perf_stop(1);
		perf_stop(0);

		if (i >= SKIP)
		{
			printf("%s %d %s %d %s %d %s %d %s %d %s %d %s %d",
				"[benchmarks][matrix]",
				i - SKIP,
				"nthreads",
				NTHREADS,
				"tnum",
				t->tnum,
				"matsize",
				MATSIZE,
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
 * @brief Matrix Multiplication Benchmark Kernel
 *
 * @param nthreads Number of working threads.
 */
static void kernel_matrix(int nthreads)
{
	int nrows;
	kthread_t tid[NTHREADS_MAX];

	/* Save kernel parameters. */
	NTHREADS = nthreads;

	nrows = MATSIZE/nthreads;

	/* Spawn threads. */
	for (int i = 0; i < nthreads; i++)
	{
		/* Initialize thread data structure. */
		tdata[i].i0 = nrows*i;
		tdata[i].in = (i == (nthreads - 1)) ? MATSIZE : (i + 1)*nrows;
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
void benchmark_matrix(void)
{
#ifndef NDEBUG

	kernel_matrix(NTHREADS_MAX);

#else

	for (int nthreads = NTHREADS_MIN; nthreads <= NTHREADS_MAX; nthreads += NTHREADS_STEP)
		kernel_matrix(nthreads);

#endif
}

#endif /* __benchmark_matrix__ */
