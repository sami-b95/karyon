/****************************************************************
 * connect.c                                                    *
 *                                                              *
 *    connect syscall.                                          *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <kernel/syscalls/utils.h>
#include <net/endian.h>
#include <net/ip.h>
#include <net/tcp.h>

/**
 * sys_connect
 */

int sys_connect(si32_t fildes, struct sockaddr *addr, socklen_t addr_len)
{
	struct fildes *fd;
	struct file *file;
	struct tcp_socket *tcp_sock;
	struct sockaddr_in *addr_in = (struct sockaddr_in*)addr;
	ret_t err;
	
	#ifdef DEBUG_SOCKETS
	printk("sys_connect(%x, %x, %x)\n", fildes, addr, addr_len);
	#endif

	*current->perrno = 0;

	if((err = fildes_check(fildes, &fd, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];

	/** Check whether we are allowed to connect. **/

	if(tcp_sock->state != TCP_CLOSED)
	{
		if((tcp_sock->state == TCP_SYN_RECEIVED
		     && !tcp_sock->active_open)
		 || tcp_sock->state == TCP_LISTEN)
		{
			*current->perrno = EINVAL;
		}
		else if(tcp_sock->state == TCP_SYN_RECEIVED
		     || tcp_sock->state == TCP_SYN_SENT)
		{
			*current->perrno = EALREADY;
		}
		else
		{
			*current->perrno = EISCONN;
		}

		return -1;
	}

	/** Bind the socket if not already done. **/

	if(!tcp_sock->source_port)
	{
		tcp_sock->source_ip = local_ip;
		tcp_sock->source_port = tcp_port_alloc();

		if(!tcp_sock->source_port)
		{
			return -1; // FIXME: errno?
		}
	}

	/** Try to establish a connection. **/

	tcp_sock->dest_ip = addr_in->sin_addr.s_addr;
	tcp_sock->dest_port = ntohs(addr_in->sin_port);
	tcp_sock->snd_una = tcp_sock->iss;
	tcp_sock->snd_nxt = tcp_sock->iss + 1;
	tcp_set_socket_state(tcp_sock, TCP_SYN_SENT, 0);

	/** Wait if possible... **/
	
	if(!(current->fildes_flags[fildes] & O_NONBLOCK))
	{
		while(tcp_sock->state != TCP_ESTABLISHED
		   && tcp_sock->state != TCP_CLOSED)
		{
			sti;
			yield;
			cli;
		}
	}

	if(tcp_sock->state == TCP_ESTABLISHED)
	{
		return 0;
	}

	if(tcp_sock->state == TCP_CLOSED)
	{
		if(tcp_sock->reset)
		{
			*current->perrno = ECONNRESET;
		}
		else
		{
			*current->perrno = ETIMEDOUT;
		}
	}
	else if(current->fildes_flags[fildes] & O_NONBLOCK)
	{
		#ifdef DEBUG_SOCKETS
		printk("letting connection in progress\n");
		#endif
		*current->perrno = EINPROGRESS;
	}

	return -1;
}
