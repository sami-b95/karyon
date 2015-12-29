/****************************************************************
 * kill.c                                                       *
 *                                                              *
 *    kill syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <kernel/int.h>
#include <kernel/libc.h> // debugging
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>

/**
 * sys_kill
 */

int sys_kill(pid_t pid, ui32_t sig)
{
	if(!proc_tab[pid].used)
	{
		return -1;
	}

	/** If pid is null, then send a signal to every process in the group of
	    the current process. **/

	if(!pid)
	{
		sys_killpg(0, sig);
	}

	/** Sending signal 0 is used to check whether a process exists. **/

	if(!sig)
	{
		return 0;
	}

	/** Modify the signal set. **/

	proc_tab[pid].sigset |= (1 << (sig - 1));

	return 0;
}
