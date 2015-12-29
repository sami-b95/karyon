/****************************************************************
 * send.c                                                       *
 *                                                              *
 *    send syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h>
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
 * sys_send
 */

ssize_t sys_send(si32_t fildes, void *buf, size_t len, ui32_t flags)
{
	struct fildes *fd;
	struct file *file;
	struct tcp_socket *tcp_sock;
	ret_t ret;

	#ifdef DEBUG_SOCKETS
	printk("sys_send(%x, %x, %x, %x)\n", fildes, buf, len, flags);
	#endif

	if((ret = fildes_check(fildes, &fd, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-ret);
		return -1;
	}

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];

	if(tcp_sock->state != TCP_ESTABLISHED
	&& tcp_sock->state != TCP_CLOSE_WAIT)
	{
		*current->perrno = EPIPE;
		return -1;
	}

	do
	{
		sti;
		yield;
		cli;
		ret = tcp_data_out(tcp_sock, buf, len);
	} while(!(current->fildes_flags[fildes] & O_NONBLOCK)
	        && ret == -ENOBUFS);

	if(ret == ENOBUFS)
	{
		*current->perrno = EWOULDBLOCK;
		return -1;
	}

	*current->perrno = (ui32_t)(-ret);
	return len;
}
