/****************************************************************
 * gsocknam.c                                                   *
 *                                                              *
 *    getsockname syscall.                                      *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <kernel/syscalls/utils.h>
#include <net/endian.h>
#include <net/tcp.h>

/**
 * sys_getsockname
 */

int sys_getsockname(si32_t fildes, struct sockaddr *addr, socklen_t *len)
{
	struct file *file;
	struct tcp_socket *tcp_sock;
	struct sockaddr_in *addr_in = (struct sockaddr_in*)addr;
	ret_t err;

	if((err = fildes_check(fildes, 0, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	if(*len < sizeof(struct sockaddr_in))
	{
		*current->perrno = EINVAL;
		return -1;
	}

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];

	addr_in->sin_family = AF_INET;
	addr_in->sin_port = htons(tcp_sock->source_port);
	addr_in->sin_addr.s_addr = tcp_sock->source_ip;
	*len = sizeof(struct sockaddr_in);

	return 0;
}
