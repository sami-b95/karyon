/****************************************************************
 * listen.c                                                     *
 *                                                              *
 *    listen syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/isr.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <kernel/syscalls/utils.h>
#include <net/tcp.h>

/**
 * sys_listen
 */

int sys_listen(si32_t fildes, ui32_t backlog)
{
	struct file *file;
	struct tcp_socket *tcp_sock;
	ret_t err;

	if((err = fildes_check(fildes, 0, &file, 1, FS_TCPSOCKFS)) != OK)
	{
		*current->perrno = (ui32_t)(-err);
		return -1;
	}

	tcp_sock = &tcp_sock_tab[file->data.tcp_sock_id];
	tcp_set_socket_state(tcp_sock, TCP_LISTEN, 0);

	return 0;
}
