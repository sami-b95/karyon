/****************************************************************
 * accept.c                                                     *
 *                                                              *
 *    accept syscall.                                           *
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
#include <kernel/syscall.h>
#include <kernel/types.h>
#include <kernel/syscalls/utils.h>
#include <net/endian.h>
#include <net/tcp.h>

/**
 * sys_accept
 */

si32_t sys_accept(si32_t fildes, struct sockaddr *addr, socklen_t *addr_len)
{
	struct tcp_request *req;
	struct fildes *fd;
	struct file *file;
	struct tcp_socket *tcp_sock, *tcp_sock2;
	si32_t tcp_sock_id;
	ret_t err;

	if(addr_len && *addr_len < sizeof(struct sockaddr))
	{
		*current->perrno = EINVAL;
		return -1;
	} 

	if((err = fildes_check(fildes, &fd, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];

	/** Try to dequeue an incoming connection. **/

	do
	{
		sti;
		yield;
		cli;

		if(tcp_sock->state != TCP_LISTEN)
		{
			*current->perrno = EINVAL;
			return -1;
		}

		req = tcp_request_pop(tcp_sock);
	} while(!req && !(current->fildes_flags[fildes] & O_NONBLOCK));

	if(!req)
	{
		*current->perrno = EWOULDBLOCK;
		return -1;
	}

	/** Create a new socket to handle the connection. Pretend we have sent
	    a SYN-ACK which has timed out. **/

	tcp_sock_id = tcp_socket_alloc();

	if(tcp_sock_id == -1)
	{
		if(tcp_request_push(tcp_sock,
		                    tcp_sock->source_ip,
		                    tcp_sock->dest_ip,
		                    tcp_sock->source_port,
		                    tcp_sock->dest_port,
		                    tcp_sock->irs) != OK)
		{
			panic("incoming connections queue possibly corrupted");
		}

		*current->perrno = ENFILE; // FIXME
		return -1;
	}

	tcp_sock2 = &tcp_sock_tab[tcp_sock_id];

	*tcp_sock2 = *tcp_sock;
	tcp_sock2->source_ip = req->dest_ip;
	tcp_sock2->dest_ip = req->source_ip;
	tcp_sock2->dest_port = req->source_port;
	tcp_sock2->snd_una = tcp_sock2->iss;
	tcp_sock2->snd_nxt = tcp_sock2->iss + 1;
	tcp_sock2->irs = req->irs;
	tcp_sock2->rcv_nxt = req->irs + 1;
	tcp_set_socket_state(tcp_sock2, TCP_SYN_RECEIVED, 0);

	/** Provide the information about the new connection. **/

	if(addr)
	{
		struct sockaddr_in *addr_in = (void*)addr;

		if(!addr_len)
		{
			*current->perrno = EINVAL;
			return -1;
		}

		addr_in->sin_family = AF_INET;
		addr_in->sin_port = htons(req->source_port);
		addr_in->sin_addr.s_addr = req->source_ip;
		memset(&addr_in->sin_zero, 0, 8);
		*addr_len = sizeof(struct sockaddr_in);
	}

	return 0;
}
