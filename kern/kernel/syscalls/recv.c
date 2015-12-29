/****************************************************************
 * recv.c                                                       *
 *                                                              *
 *    recv syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/fifo.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <kernel/syscalls/utils.h>
#include <net/tcp.h>

/**
 * sys_recv
 */

ssize_t sys_recv(si32_t fildes, void *buf, size_t len, ui32_t flags)
{
	struct fildes *fd;
	struct file *file;
	struct tcp_socket *tcp_sock;
	ssize_t ret;
	ret_t err;

	#ifdef DEBUG_SOCKETS
	printk("sys_recv(%x, %x, %x, %x)\n", fildes, buf, len, flags);
	#endif

	if((err = fildes_check(fildes, &fd, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		ret = -1;
		goto end;
	}

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];

	if(tcp_sock->state == TCP_CLOSED
	|| tcp_sock->state == TCP_SYN_SENT
	|| tcp_sock->state == TCP_SYN_RECEIVED)
	{
		*current->perrno = ENOTCONN;
		ret = -1;
		goto end;
	}
	else if((tcp_sock->state == TCP_CLOSE_WAIT
	      || tcp_sock->state == TCP_CLOSING
	      || tcp_sock->state == TCP_TIME_WAIT
	      || tcp_sock->state == TCP_LAST_ACK)
	     && !tcp_sock->rx_fifo.to_read)
	{
		*current->perrno = 0;
		ret = 0;
		goto end;
	}

	/*if((tcp_sock->state == TCP_CLOSED || tcp_sock->state > TCP_ESTABLISHED)
	&& !tcp_sock->rx_fifo.to_read)
	{
		ret = 0;
		goto end;
	}
	else if(tcp_sock->state < TCP_ESTABLISHED)
	{
		*current->perrno = ENOTCONN;
		ret = -1;
		goto end;
	}*/

	#ifdef DEBUG_SOCKETS
	printk("socket state: %x\n", tcp_sock->state);
	#endif

	do
	{
		sti;
		yield;
		cli;
		#ifdef DEBUG_SOCKETS
		printk("reading...: %x available\n",
		       tcp_sock->rx_fifo.to_read);
		#endif
		ret = tcp_data_push(tcp_sock, buf, len);
		#ifdef DEBUG_SOCKETS
		printk("read bytes: %x\n", ret);
		#endif
	} while(!(current->fildes_flags[fildes] & O_NONBLOCK) && !ret);

	#ifdef DEBUG_SOCKETS
	printk("out of receive loop\n");
	#endif

	if(!ret)
	{
		*current->perrno = EWOULDBLOCK;
		ret = -1;
	}

	end:
		return ret;
}
