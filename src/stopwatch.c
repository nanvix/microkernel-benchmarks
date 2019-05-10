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

#ifdef __i486__

/**
 * @brief Dummy clock_read().
 */
uint64_t clock_read(void)
{
	return (0);
}

#endif

/**
 * @brief Stopwatch delay.
 */
static uint64_t stopwatch_delay = 0;

/**
 * @todo TODO provide a detailed description for this function.
 */
void stopwatch_init(void)
{
	uint64_t t0 = clock_read();
	uint64_t t1 = clock_read();

	stopwatch_delay = t1 - t0;
}

/**
 * @brief Computes the time difference on the stopwatch.
 */
uint64_t stopwatch_diff(uint64_t t0, uint64_t t1)
{
	return ((t1 - t0 - stopwatch_delay));
}
