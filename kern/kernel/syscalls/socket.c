/****************************************************************
 * socket.c                                                     *
 *                                                              *
 *    socket syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/types.h>
#include <net/tcp.h>

/**
 * sys_socket
 */

int sys_socket(ui32_t domain, ui32_t type, ui32_t proto)
{
	ino_t inum;
	si32_t fildes;
	struct fildes *fd;
	si32_t sock_id;

	#ifdef DEBUG_SOCKETS
	printk("sys_socket(%x, %x, %x)\n", domain, type, proto);
	#endif

	/** Check domain, type and protocol. **/

	if(domain != AF_INET || type != SOCK_STREAM)
	{
		*current->perrno = ENOSYS;

		goto fail1;
	}

	/** Allocate a file structure. **/

	inum = file_alloc();

	if(!inum)
	{
		goto fail2;
	}

	/** Allocate a socket structure. **/

	sock_id = tcp_socket_alloc();

	if(sock_id == -1)
	{
		goto fail2;
	}

	/** Allocate a file descriptor. **/

	fildes = fildes_alloc(0);

	if(fildes == -1)
	{
		goto fail3;
	}

	/** Initialize the file. **/

	file_tab[inum - 1].fs = FS_TCPSOCKFS;
	file_tab[inum - 1].data.tcp_sock_id = sock_id;

	/** Initialize the file descriptor. **/

	fd = current->pfildes_tab[fildes];
	current->fildes_flags[fildes] = O_RDWR;
	fd->off = 0;
	fd->inum = inum;

	/** Initialize the socket. **/

	tcp_socket_init(&tcp_sock_tab[sock_id]);

	/** Reference the file. **/

	file_ref(inum);

	return fildes;

	fail3:
		tcp_socket_free(sock_id);
	fail2:
		*current->perrno = ENOMEM;
	fail1:
		#ifdef DEBUG_SOCKETS
		panic("failed creating socket\n");
		#endif
		return -1;
}
