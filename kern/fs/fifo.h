#ifndef _FIFO_H_
#define _FIFO_H_

#include <config.h>
#include <kernel/types.h>

/** FIFO and pipe structures. **/

struct fifo
{
	off_t read;
	off_t write;
	ui8_t rbuf[RBUF_SIZE];
	count_t to_read;
};

struct pipe
{
	bool_t used;
	count_t readers;
	count_t writers;
	struct fifo fifo;
};

/** Functions. **/

ret_t fifo_read_byte(struct fifo *fifo, ui8_t *byte, bool_t block);
/****************************************************************/
size_t fifo_read(struct fifo *fifo, void *buf, size_t size, bool_t block);
/****************************************************************/
ret_t fifo_write_byte(struct fifo *fifo, ui8_t byte, bool_t block);
/****************************************************************/
ret_t fifo_write(struct fifo *fifo, void *buf, size_t size, bool_t block);
/****************************************************************/
void fifo_pop_last(struct fifo *fifo);
/****************************************************************/
void fifo_flush(struct fifo *fifo);
/****************************************************************/
count_t fifo_left(struct fifo *fifo);

#endif
