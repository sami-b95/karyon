/****************************************************************
 * bind.c                                                       *
 *                                                              *
 *    bind syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <kernel/syscalls/utils.h>
#include <net/endian.h>
#include <net/ip.h>
#include <net/tcp.h>

/**
 * sys_bind
 */

int sys_bind(si32_t fildes, struct sockaddr *addr, socklen_t addr_len)
{
	struct sockaddr_in *addr_in;
	struct file *file;
	struct tcp_socket *tcp_sock;
	ret_t err;

	if((err = fildes_check(fildes, 0, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	addr_in = (struct sockaddr_in*)addr;

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];

	if(tcp_port_is_used(ntohs(addr_in->sin_port)))
	{
		*current->perrno = EADDRINUSE;
		return -1;
	}

	tcp_sock->source_ip = addr_in->sin_addr.s_addr;
	tcp_sock->source_port = ntohs(addr_in->sin_port);

	return 0;
}
