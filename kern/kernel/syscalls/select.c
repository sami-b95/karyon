/****************************************************************
 * select.c                                                     *
 *                                                              *
 *    select syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/file.h>
#include <fs/tty.h>
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_select
 */

si32_t sys_select(ui32_t nfds,
                  fd_set *readfds,
                  fd_set *writefds,
                  fd_set *exceptfds,
                  struct timeval *timeout)
{
	si32_t fildes;
	struct fildes *fd;
	struct file *file;
	struct tcp_socket *tcp_sock;
	si32_t ret = 0;
	clock_t timeout_set_tics = tics;
	clock_t timeout_tics
	 = timeout ? (tics
	            + timeout->tv_sec * CLK_FREQ
	            + timeout->tv_usec * CLK_FREQ / 1000000 + 1) // FIXME
	           : 0;
	ret_t err;

	// WARNING: stack overflow risk if NR_FILDES_PER_PROC is too big
	fd_set readfds_in, writefds_in, exceptfds_in;

	/** Check for nfds validity. **/

	if(nfds > NR_FILDES_PER_PROC)
	{
		ret = -1;
		*current->perrno = EBADF;
		goto end;
	}

	/** Save the input file descriptors sets. **/

	if(readfds)
	{
		readfds_in = *readfds;
	}

	if(writefds)
	{
		writefds_in = *writefds;
	}

	if(exceptfds)
	{
		exceptfds_in = *exceptfds;
	}

	/** Check whether readfds descriptors (if any provided) are ready for
	    reading. **/

	retry:

	if(readfds)
	{
		for(fildes = 0; fildes < nfds; fildes++)
		{
			if(FD_ISSET(fildes, &readfds_in))
			{
				/** If the file descriptor is not valid, return
				    an error. **/

				if((err = fildes_check(fildes,
				                       &fd,
				                       &file,
				                       1,
				                       FS_NOFS)) != OK)
				{
					*current->perrno = (ui32_t)(-err);
					ret = -1;
					goto end;
				}

				/** Filesystem-specific operations. **/

				if(fd->inum == 1)
				{
					/** Handle the special case of the
					    tty. **/

					if(tty_ififo.to_read
					|| tty_ififo2.to_read)
					{
						ret++;
						FD_SET(fildes, readfds);
						FD_CLR(fildes, &readfds_in);
					}
					else
					{
						FD_CLR(fildes, readfds);
					}
				}
				else if(file->fs == FS_TCPSOCKFS)
				{
					tcp_sock = &tcp_sock_tab
					             [file->data.tcp_sock_id];
					#ifdef DEBUG_SOCKETS
					debug = 1;
					printk("select: sock fildes (r)\n");
					#endif

					if(tcp_sock->state == TCP_CLOSE_WAIT
					|| tcp_sock->state == TCP_CLOSING
					|| tcp_sock->state == TCP_TIME_WAIT
					|| tcp_sock->state == TCP_LAST_ACK
					|| tcp_sock->rx_fifo.to_read)
					{
						ret++;
						FD_SET(fildes, readfds);
						FD_CLR(fildes, &readfds_in);
					}
					else	
					{
						FD_CLR(fildes, readfds);
					}
				}
				else
				{
					/** Handle the remaining cases. **/

					ret++;
					FD_SET(fildes, readfds);
					FD_CLR(fildes, &readfds_in);
				}
			}
		}
	}

	/** Check whether descriptors are ready for writing. **/

	if(writefds)
	{
		/** TODO Handle terminal. **/

		for(fildes = 0; fildes < nfds; fildes++)
		{
			if(FD_ISSET(fildes, &writefds_in))
			{
				/** If the file descriptor is not valid, return
				    an error. **/

				if((err = fildes_check(fildes,
				                       &fd,
				                       &file,
				                       1,
				                       FS_NOFS)) != OK)
				{
					*current->perrno = (ui32_t)(-err);
					ret = -1;
					goto end;
				}

				/** Filesystem-specific operations. **/

				if(file->fs == FS_TCPSOCKFS)
				{
					#ifdef DEBUG_SOCKETS
					debug = 1;
					printk("select: sock fildes (w)\n");
					#endif
					tcp_sock = &tcp_sock_tab
					            [file->data.tcp_sock_id];

					if((tcp_sock->state == TCP_ESTABLISHED
					 || tcp_sock->state == TCP_CLOSE_WAIT)
					&& tcp_sock->free_buf)
					{
						ret++;
						FD_SET(fildes, writefds);
						FD_CLR(fildes, &writefds_in);
					}
					else
					{
						FD_CLR(fildes, writefds);
						*current->perrno = EALREADY;
					}
				}
				else
				{
					ret++;
					FD_SET(fildes, writefds);
					FD_CLR(fildes, &writefds_in);
				}
			}
		}
	}

	/** TODO exceptfds **/

	if((!timeout || byte_in_win(tics, timeout_set_tics, timeout_tics))
	&& !ret)
	{
		sti;
		yield;
		cli;
		goto retry;
	}

	#ifdef DEBUG_SOCKETS
	if(debug)
	{
		printk("select returns %x\n", ret);
	}
	#endif

	if(ret) // FIXME: Not sure about this if
	{
		*current->perrno = 0;
	}

	end:
		return ret;
}
