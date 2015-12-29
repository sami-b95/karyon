/****************************************************************
 * sigsusp.c                                                    *
 *                                                              *
 *    sigsuspend syscall.                                       *
 *                                                              *
 ****************************************************************/

#include <kernel/int.h>
#include <kernel/libc.h>
#include <kernel/process.h>
#include <kernel/signal.h>

/**
 * sys_sigsuspend
 */

int sys_sigsuspend(sigset_t *mask)
{
	return -1;
}


