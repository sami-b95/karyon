/****************************************************************
 * killpg.c                                                     *
 *                                                              *
 *    killpg syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <kernel/libc.h> // debugging
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/syscall.h>
#include <kernel/types.h>

/**
 * sys_killpg
 */

int sys_killpg(pid_t pgid, ui32_t sig)
{
	pid_t epgid; // effective process group
	count_t killed;

	#ifdef DEBUG
	printk("sys_killpg(%x, %x)\n", pgid, sig);
	delay(20000);
	#endif

	/** Get the effective process group. **/

	if(!pgid)
	{
		epgid = current->pgid;
	}
	else
	{
		epgid = pgid;
	}

	/** Send the signal to every process belonging to the group (if the
	    group is not empty). **/

	killed = signal_send_pgrp(sig, epgid);

	/** If no process could be killed (i.e the group is empty), return an
	    error. **/

	if(!killed)
	{
		return -1;
	}

	return 0;
}
