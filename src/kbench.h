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

#ifndef _KBENCH_H_
#define _KBENCH_H_

	#include <stdint.h>

	/**
	 * @brief Number of benchmark iterations.
	 */
	#ifdef NDEBUG
		#define NITERATIONS 30
	#else
		#define NITERATIONS 1
	#endif

	/**
	 * @brief Iterations to skip on warmup.
	 */
	#define SKIP 10

	/**
	 * @brief Casts something to a uint32_t.
	 */
	#define UINT32(x) ((uint32_t)((x) & 0xffffffff))

	/**
	 * @brief Alias for kprintf().
	 */
	#define printf(fmt, ...) kprintf(fmt, ##__VA_ARGS__)

/*============================================================================*
 * Stopwatch                                                                  *
 *============================================================================*/

	/**
	 * @brief Initializes the stopwatch.
	 */
	extern void stopwatch_init(void);

	/**
	 * @brief Computes the time difference on the stopwatch.
	 *
	 * @param t0 First timestamp.
	 * @param t1 Second timestamp.
	 *
	 * @returns The difference between t0 and t1.
	 */
	extern uint64_t stopwatch_diff(uint64_t t0, uint64_t t1);

	/**
	 * @brief Reads the stopwatch.
	 *
	 * @param The current timestamp of the stopwatch.
	 */
	static inline uint64_t stopwatch_read(void)
	{
		return (clock_read());
	}

/*============================================================================*
 * Memory Functions                                                           *
 *============================================================================*/

	/**
	 * @brief Fills words in memory.
	 *
	 * @param ptr Pointer to target memory area.
	 * @param c   Character to use.
	 * @param n   Number of bytes to be set.
	 */
	static inline void memfill(word_t *ptr, word_t c, size_t n)
	{
		word_t *p;

		p = ptr;

		/* Set words. */
		for (size_t i = 0; i < n; i++)
			*p++ = c;
	}

	/**
	 * @brief Copy words in memory.
	 *
	 * @param dest Target memory area.
	 * @param src  Source memory area.
	 * @param n    Number of bytes to be copied.
	 */
	static inline void memcopy(word_t *dest, const word_t *src, size_t n)
	{
		word_t *d;       /* Write pointer. */
		const word_t* s; /* Read pointer.  */

		s = src;
		d = dest;

		/* Copy words. */
		for (size_t i = 0; i < n; i++)
			*d++ = *s++;
	}

/*============================================================================*
 * Kernel benchmark                                                           *
 *============================================================================*/

	/**
	 * @brief Performance event.
	 */
	struct perf_event
	{
		const char *name; /**< Event name.   */
		int num;          /**< Event number. */
	};

	/**
	 * @brief Performance events.
	 */
	extern struct perf_event perf_events[8];

	/**
	 * @name Benchmarks
	 */
	/**@{*/
	extern void benchmark_perf(void);
	extern void benchmark_kcall_local(void);
	extern void benchmark_kcall_remote(void);
	extern void benchmark_matrix(void);
	extern void benchmark_buffer(void);
	/**@}*/

#endif /* _KBENCH_H_ */
