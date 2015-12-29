/****************************************************************
 * fifo.c                                                       *
 *                                                              *
 *    FIFO implementation.                                      *
 *                                                              *
 ****************************************************************/

#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/libc.h>
#include <kernel/panic.h>

#include "fifo.h"

/** WARNING: Interrupts must be disabled when calling these functions. **/

/**
 * fifo_read_byte
 */

ret_t fifo_read_byte(struct fifo *fifo, ui8_t *byte, bool_t block)
{
	if(!fifo->to_read)
	{
		if(block)
		{
			/** If we can block, wait until the buffer contains at
			    least one byte. **/

			while(1)
			{
				sti;
				yield;
				//delay(FIFO_DELAY);
				cli;

				if(fifo->to_read)
				{
					break;
				}
			}
		}
		else
		{
			/** Else return an error to indicate that no byte could
			    be read. **/

			return -1; // FIXME: Use a constant defined in errno.h.
		}
	}

	/** Read first byte. **/

	*byte = fifo->rbuf[fifo->read];

	/** Update the read cursor and the number of bytes to read. **/

	fifo->read++;

	if(fifo->read == RBUF_SIZE)
	{
		fifo->read = 0;
	}
	else if(fifo->read > RBUF_SIZE)
	{
		panic("fifo read cursor is %x", fifo->read);
	}

	fifo->to_read--;

	return OK;
}

/**
 * fifo_read
 */

// NOTE: This function reads _at most_ size bytes.

size_t fifo_read(struct fifo *fifo, void *buf, size_t size, bool_t block)
{
	size_t i;

	for(i = 0; i < size; i++)
	{
		if(fifo_read_byte(fifo, buf + i, block && !i) != OK)
		{
			break;
		}
	}

	return i;
}

/**
 * fifo_write_byte
 */

ret_t fifo_write_byte(struct fifo *fifo, ui8_t byte, bool_t block)
{
	if(fifo->to_read == RBUF_SIZE)
	{
		if(block)
		{
			/** If we can block, wait until we can write to the
			    buffer without overriding its content. **/

			while(1)
			{
				sti;
				yield;
				//delay(FIFO_DELAY);
				cli;

				if(fifo->to_read < RBUF_SIZE)
				{
					break;
				}
			}
		}
		else
		{
			/** Else return an error code to indicate that the byte
			    could not be written. **/

			return -EWOULDBLOCK;
		}
	}

	/** Write the byte. **/

	fifo->rbuf[fifo->write] = byte;

	/** Update the write cursor and the number of bytes to read. **/

	fifo->write++;

	if(fifo->write == RBUF_SIZE)
	{
		fifo->write = 0;
	}
	else if(fifo->write > RBUF_SIZE)
	{
		panic("fifo write cursor is %x", fifo->write);
	}

	fifo->to_read++;

	return OK;
}

/**
 * fifo_write
 */

ret_t fifo_write(struct fifo *fifo, void *buf, size_t size, bool_t block)
{
	size_t i;
	ret_t ret;

	for(i = 0; i < size; i++)
	{
		ret = fifo_write_byte(fifo, *(ui8_t*)(buf + i), block);

		if(ret != OK)
		{
			break;
		}
	}

	return ret;
}

/**
 * fifo_pop_last
 */

void fifo_pop_last(struct fifo *fifo)
{
	if(fifo->to_read)
	{
		/*if(fifo->write)
		{
			fifo->write--;
		}
		else
		{
			fifo->write = RBUF_SIZE - 1;
		}*/

		fifo->write = (fifo->write + RBUF_SIZE - 1) % RBUF_SIZE;
		fifo->to_read--;
	}
}

/**
 * fifo_flush
 */

void fifo_flush(struct fifo *fifo)
{
	fifo->read = fifo->write = 0;
	fifo->to_read = 0;
}

/**
 * fifo_left
 */

count_t fifo_left(struct fifo *fifo)
{
	return RBUF_SIZE - fifo->to_read;
}
