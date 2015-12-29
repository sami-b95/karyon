/****************************************************************
 * pipe2.c                                                      *
 *                                                              *
 *    pipe2 syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <fs/fifo.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/process.h>
#include <kernel/types.h>

/** Pipe table. **/

struct pipe pipe_tab[NR_PIPES] = {
	{
		.used = 0
	}
};

/**
 * sys_pipe2
 */

int sys_pipe2(si32_t pipefd[2], ui32_t flags)
{
	ino_t inum;
	si32_t fildes_read, fildes_write;
	struct fildes *fd_read, *fd_write;
	si32_t pipe_id;
	struct pipe *pipe;

	/** Allocate a file structure. **/

	inum = file_alloc();

	if(!inum)
	{
		goto fail1;
	}

	/** Allocate a pipe structure. **/

	for(pipe_id = 0; pipe_id < NR_PIPES; pipe_id++)
	{
		pipe = &pipe_tab[pipe_id];

		if(!pipe->used)
		{
			break;
		}
	}

	if(pipe_id == NR_PIPES)
	{
		*current->perrno = ENFILE;
		goto fail2;
	}

	/** Allocate the reading file descriptor. **/

	fildes_read = fildes_alloc(0);

	if(fildes_read == -1)
	{
		*current->perrno = EMFILE;
		goto fail2;
	}

	/** Allocate the writing file descriptor. **/

	fildes_write = fildes_alloc(0);

	if(fildes_write == -1)
	{
		*current->perrno = EMFILE;
		goto fail3;
	}

	/** Initialize the file. **/

	file_tab[inum - 1].fs = FS_PIPEFS;
	file_tab[inum - 1].data.pipe_id = pipe_id;

	/** Initialize file descriptors. **/

	fd_read = current->pfildes_tab[fildes_read];
	current->fildes_flags[fildes_read]
	 = O_RDONLY | (flags & (O_CLOEXEC | O_NONBLOCK));
	fd_read->off = 0;
	fd_read->inum = inum;

	fd_write = current->pfildes_tab[fildes_write];
	current->fildes_flags[fildes_write]
	 = O_WRONLY | (flags & (O_CLOEXEC | O_NONBLOCK));
	fd_write->off = 0;
	fd_write->inum = inum;

	/** Initialize the pipe. **/

	pipe->used = 1;
	pipe->readers = pipe->writers = 1;
	fifo_flush(&pipe->fifo);

	/** Reference the file (twice). **/

	file_ref(inum);
	file_ref(inum);

	pipefd[0] = fildes_read;
	pipefd[1] = fildes_write;

	return 0;

	fail3:
		fildes_unref(fildes_read);
	fail2:
		file_ref(inum);
		file_unref(inum);
	fail1:
		return -1;
}
