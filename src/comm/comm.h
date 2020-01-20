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

#ifndef _COMM_H_
#define _COMM_H_

	#include <nanvix/runtime/stdikc.h>
	#include <nanvix/sys/dev.h>
    #include <nanvix/ulib.h>
    #include <kbench.h>

	/**
     * @name Collective communication routines functions
     */
    /**@{*/
    extern int do_broadcast(const int *, int, int, int);
    extern int do_gather(const int *, int, int, int);
    extern int do_allgather(const int *, int, int, int);
    extern int do_pingpong(const int *, int, int, int);
    /**@}*/

    /**
     * @brief Executable routine
     */
    extern int (*enabled_routine_fn)(const int *, int, int, int);

    /**
     * @name Configuration of the routines
     */
    /**@{*/
    #define NUM_CLUSTERS 17
    extern const int nodenums[NUM_CLUSTERS];
    /**@}*/

    /**
     * @brief Portal message size
     */
    #define MESSAGE_SIZE (1 * KB)

    /**
     * @name Barrier functions
     */
    /**@{*/
    extern void barrier_setup(const int * nodes, int nnodes, int is_master);
    extern void barrier(void);
    extern void barrier_cleanup(void);
    /**@}*/

    /**
     * @brief Horizontal line.
     */
    extern const char *HLINE;

#endif
