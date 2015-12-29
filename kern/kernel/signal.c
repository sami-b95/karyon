/****************************************************************
 * signal.c                                                     *
 *                                                              *
 *    Signals implementation.                                   *
 *                                                              *
 ****************************************************************/

#include <config.h>
#ifdef DEBUG
#include <kernel/libc.h>
#endif
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/syscall.h>
#ifdef INTERRUPTIBLE_SYSCALLS
#include <fs/tty.h>
#endif

#include "signal.h"

/**
 * signal_dequeue
 */

ui32_t signal_dequeue(sigset_t sigset, sigset_t sigmask)
{
	ui32_t sig;

	sigmask &= ~(SIGKILL | SIGSTOP);
	sigset &= ~sigmask;

	for(sig = 1; sig <= 22; sig++)
	{
		if(sigset & (1 << (sig - 1)))
		{
			return sig;
		}
	}

	return 0;
}

/**
 * signal_handle
 *
 * WARNING: This function must be called from schedule_switch().
 */

void signal_handle(ui32_t sig)
{
	ui32_t *esp;
	void *handler = current->sigact[sig - 1].sa_handler;

	#ifdef DEBUG
	printk("handling signal %x\n", sig);
	#endif

	/** Clear the signal. **/

	current->sigset &= ~(1 << (sig - 1));

	/** Switch to the page directory of the current process. It may be
	    useful to interrupt a system call. **/

	asm volatile("mov %0, %%eax \n\
	              mov %%eax, %%cr3" :
	                                : "m"(current->pd)
	                                : "eax");

	/** If the process is doing a system call and is interruptible,
	    interrupt the system call. **/

	#ifdef INTERRUPTIBLE_SYSCALLS
	if(current->interruptible)
	{
		#ifdef DEBUG_SYSCALLS
		printk("interrupting syscall %x\n", current->sysc_num);
		delay(50000);
		#endif

		if(current->perrno < (ui32_t*)USER_BASE
		|| current->perrno >= (ui32_t*)USER_STACK_BASE)
		{
			panic("bad errno pointer %x", current->perrno);
		}

		*current->perrno = EINTR;
		current->regs = current->regs2;
		current->regs.eax = (ui32_t)-1;

		/** Be careful about interrupting system calls. If the process
		    was trying to read from the terminal, the terminal lock
		    must be released. **/

		if(reader_pid == current_pid)
		{
			//printk("freeing the terminal\n");
			reader_pid = 0;
		}

		current->interruptible = 0;
	}
	#endif

	/** Handle the signal. **/

	if(handler == SIG_IGN)
	{
		#ifdef DEBUG
		printk("ignore signal %x received by %x\n", sig, current_pid);
		#endif
	}
	else if(handler == SIG_DFL)
	{
		switch(sig)
		{
			case SIGHUP:
			case SIGINT:
			case SIGQUIT:
			case SIGKILL:
			case SIGSEGV:
				sys_exit(sig);
				break;

			case SIGSTOP: case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if(!current->parent)
				{
					panic("process without parent");
				}

				current->parent->sigset
				 |= (1 << (SIGCHLD - 1));
				current->parent->dead_son_pid = current_pid;
				current->parent->dead_son_status
				 = (ui8_t)sig << 8 | 0x7f;
				current->status = (ui8_t)sig << 8 | 0x7f;
				current->reported = 0;
				current->state = PROC_SUSPENDED;
				schedule_switch(0);
				break;

			default:
				break;
		}

		#ifdef DEBUG
		printk("default handling for signal %x received by %x\n",
		       sig,
		       current_pid);
		#endif
	}
	else
	{
		//if(current->regs.cs == 0x08) return;

		esp = (ui32_t*)current->regs.esp;
		esp -= 21;

		esp[20] = current->sigmask;
		esp[19] = current->regs.ss;
		esp[18] = current->regs.esp;
		esp[17] = current->regs.eflags;
		esp[16] = current->regs.cs;
		esp[15] = current->regs.eip;
		esp[14] = current->regs.eax;
		esp[13] = current->regs.ecx;
		esp[12] = current->regs.edx;
		esp[11] = current->regs.ebx;
		esp[10] = current->regs.ebp;
		esp[9] = current->regs.esi;
		esp[8] = current->regs.edi;
		esp[7] = current->regs.ds;
		esp[6] = current->regs.es;
		esp[5] = current->regs.fs;
		esp[4] = current->regs.gs;
		esp[3] = 0x0030cd00;
		esp[2] = 0x000021b8;
		esp[1] = sig;
		esp[0] = (ui32_t)&esp[2];

		current->regs.esp = (ui32_t)esp;
		current->regs.eip = (ui32_t)handler;

		/** Update signal mask if required **/

		if(!(current->sigact[sig - 1].sa_flags & SA_NODEFER))
		{
			current->sigmask |= (1 << (sig - 1));
		}

		#ifdef DEBUG
		printk("jump to signal handler with eip = %x, cs = %x,\n",
		       current->regs.eip, current->regs.cs);
		printk("jump to signal handler with esp = %x\n",
		       current->regs.esp);
		delay(2000);
		#endif

		/*panic("procedure %x for signal %x received by %x",
		      (ui32_t)handler, sig, current_pid);*/
	}
}

/**
 * signal_send_pgrp
 */

count_t signal_send_pgrp(ui32_t sig, pid_t pgid)
{
	pid_t pid;
	count_t killed = 0;

	for(pid = 0; pid < NR_PROC; pid++)
	{
		if(proc_tab[pid].used && proc_tab[pid].pgid == pgid)
		{
			proc_tab[pid].sigset |= (1 << (sig - 1));
			killed++;
		}
	}

	return killed;
}
