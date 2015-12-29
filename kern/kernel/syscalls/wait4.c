/****************************************************************
 * wait4.c                                                      *
 *                                                              *
 *    wait4 syscall.                                            *
 *                                                              *
 ****************************************************************/

#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/signal.h>
#include <kernel/types.h>

#define WNOHANG		1
#define WUNTRACED	2

/**
 * sys_wait4
 */

pid_t sys_wait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	struct process *proc;
	bool_t matching_id = 0;

	do
	{
		sti;
		yield;
		cli;

		if(!current->first_son)
		{
			break;
		}

		/** Look for an unwaited-for son. **/

		proc = current->first_son;

		do
		{
			if(!proc->used)
			{
				panic("son process %x is unused", proc->pid);
			}

			/** Check pid or pgid. **/

			if((!(pid == 0 && proc->pgid == current->pgid)
			 && !(pid < 0 && proc->pgid == -pid)
			 && !(pid > 0 && proc->pid == pid)
			 && pid != -1))
			{
				goto next_son;
			}

			matching_id = 1;

			/** I. **/

			if((!(proc->state == PROC_ZOMBIE)
			 && !(proc->state == PROC_SUSPENDED
			       && (options & WUNTRACED)))
			|| proc->reported)
			{
				goto next_son;
			}

			if(proc->state == PROC_ZOMBIE)
			{
				/** We have a zombie process. Remove it from
				    the sons list of the current process and
				    free the process table entry. **/

				if(current->first_son == proc)
				{
					if(proc->next_sibling == proc)
					{
						current->first_son = 0;
					}
					else
					{
						current->first_son
						 = proc->next_sibling;
					}
				}
				/* WARNING: recent (10/13) else
				{*/
					proc->next_sibling->prev_sibling
					 = proc->prev_sibling;
					proc->prev_sibling->next_sibling
					 = proc->next_sibling;
				//}

				proc->used = 0;
			}

			/** Report the status of the process if possible. **/

			if(status)
			{
				*status = proc->status;
			}

			if(rusage) // FIXME
			{
				rusage->ru_utime.tv_sec = 0;
				rusage->ru_utime.tv_usec = 10;
				rusage->ru_stime.tv_sec = 0;
				rusage->ru_stime.tv_usec = 10;
			}

			proc->reported = 1;

			return proc->pid;

			next_son:
				proc = proc->next_sibling;
		} while(proc != current->first_son);
	} while(!(options & WNOHANG));

	if(matching_id)
	{
		return 0;
	}

	*current->perrno = ECHILD;

	return -1;
}
