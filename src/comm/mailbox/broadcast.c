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
#include <nanvix/sys/mailbox.h>

#include "../comm.h"

#if __TARGET_HAS_MAILBOX

static char message[MAILBOX_MSG_SIZE];

static inline void do_master(const int * nodes, int nslaves)
{
	int local;
	int outbox;

	local = nodes[0];

	umemset(message, local, MAILBOX_MSG_SIZE);

	for (unsigned i = 1; i <= NITERATIONS; ++i)
	{
		uprintf("Iteration %d/%d", i, NITERATIONS);

		barrier();

		for (int j = 1; j <= nslaves; ++j)
		{
			/* Opens connector. */
			KASSERT((outbox = kmailbox_open(nodes[j])) >= 0);

			/* Sends the message for current slave. */
			KASSERT(kmailbox_write(outbox, message, MAILBOX_MSG_SIZE) == MAILBOX_MSG_SIZE);

			/* Closes connector. */
			KASSERT(kmailbox_close(outbox) == 0);
		}
	}
}

static inline void do_slave(const int * nodes, int index)
{
	int inbox;
	int local;
	int remote;

	local  = nodes[index];
	remote = nodes[0];

	for (unsigned i = 1; i <= NITERATIONS; ++i)
	{
		KASSERT((inbox = kmailbox_create(local)) >= 0);

			umemset(message, 0, MAILBOX_MSG_SIZE);

			barrier();

			KASSERT(kmailbox_read(inbox, message, MAILBOX_MSG_SIZE) == MAILBOX_MSG_SIZE);

			for (int j = 0; j < MAILBOX_MSG_SIZE; ++j)
				KASSERT(message[j] == remote);

		KASSERT(kmailbox_unlink(inbox) == 0);
	}
}

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Performance Broadcast Routine
 */
int do_broadcast(const int * nodes, int nnodes, int index, int message_size)
{
	UNUSED(message_size);

	if (index == 0)
		uprintf("[mailbox][broadcast] Init.");

	/* Runs the kernel. */
	if (index == 0)
		do_master(nodes, (nnodes - 1));
	else
		do_slave(nodes, index);

	if (index == 0)
		uprintf("[mailbox][broadcast] Finished.");

	/* Synchronizes. */
	barrier();

	if (index == 0)
		uprintf("[mailbox][broadcast] Successfuly completed.");

	return (0);
}

#else

/*============================================================================*
 * Benchmark Driver                                                           *
 *============================================================================*/

/**
 * @brief Performance Broadcast Routine
 */
int do_broadcast(const int * nodes, int nnodes, int index, int message_size)
{
	UNUSED(nodes);
	UNUSED(nnodes);
	UNUSED(index);
	UNUSED(message_size);

	return (0);
}
#endif
