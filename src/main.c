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

#include <nanvix.h>
#include "kbench.h"

/**
 * @brief Lunches user-land testing units.
 *
 * @param argc Argument counter.
 * @param argv Argument variables.
 */
int main(int argc, const char *argv[])
{
	((void) argc);
	((void) argv);

	stopwatch_init();

	kprintf("--------------------------------------------------------------------------------");

#ifdef __benchmark_perf__
	benchmark_perf();
#endif

#ifdef __benchmark_kcall_local__
	benchmark_kcall_local();
#endif

#ifdef __benchmark_kcall_remote__
	benchmark_kcall_remote();
#endif

#ifdef __benchmark_matrix__
	benchmark_matrix();
#endif

#ifdef __benchmark_memmove__
	benchmark_memmove();
#endif

#ifdef __benchmark_buffer__
	benchmark_buffer();
#endif

#ifdef __benchmark_fork_join__
	benchmark_fork_join();
#endif

#ifdef __benchmark_upcall__
	benchmark_upcall();
#endif

#ifdef __benchmark_gauss__
	benchmark_gauss();
#endif

#ifdef __benchmark_tsp__
	benchmark_tsp();
#endif

	kprintf("--------------------------------------------------------------------------------");

	/* Shutdown. */
	shutdown();

	return (0);
}
