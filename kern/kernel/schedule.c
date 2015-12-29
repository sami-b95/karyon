/****************************************************************
 * schedule.c                                                   *
 *                                                              *
 *    Scheduling functions.                                     *
 *                                                              *
 ****************************************************************/
#include <config.h>
#include <kernel/errno.h>
#include <kernel/gdt.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#ifdef INTERRUPTIBLE_SYSCALLS
#include <fs/tty.h>
#endif

#include "schedule.h"

/**
 * schedule
 *
 * called from an ISR
 *
 */

void schedule()
{
	ui32_t *ebp_isr; // EBP of the C interrupt service routine.
	pid_t pid;

	asm volatile("mov (%%ebp), %%eax \n\
	              mov %%eax, %0" : "=m"(ebp_isr)
	                             :
	                             : "eax", "memory");

	if(!nproc)
	{
		/*#ifdef DEBUG
		printk("no process\n");
		#endif*/

		return;
	}
	else if(nproc == 1)
	{
		#ifdef DEBUG
		printk("one process\n");
		#endif

		return;
	}
	else
	{
		/** Save the context of the interrupted process. **/

		schedule_save_regs(&current->regs, ebp_isr, 0);

		/*current->regs.gs = ebp_isr[2];
		current->regs.fs = ebp_isr[3];
		current->regs.es = ebp_isr[4];
		current->regs.ds = ebp_isr[5];
		current->regs.edi = ebp_isr[6];
		current->regs.esi = ebp_isr[7];
		current->regs.ebp = ebp_isr[8];
		current->regs.ebx = ebp_isr[10];
		current->regs.edx = ebp_isr[11];
		current->regs.ecx = ebp_isr[12];
		current->regs.eax = ebp_isr[13];
		current->regs.eip = ebp_isr[14];
		current->regs.cs = ebp_isr[15];
		current->regs.eflags = ebp_isr[16];

		if(ebp_isr[15] == 0x08)
		{
			current->regs.esp = ebp_isr[9] + 12;
			current->regs.ss = 0x18;
			if(current->regs.esp >= 0xf0000000)
			{
				panic("bad saved esp %x (process %x)",
				       current->regs.esp, current_pid);
			}
		}
		else
		{
			current->regs.esp = ebp_isr[17];
			current->regs.ss = ebp_isr[18];
		}*/
	}

	/** Choose a new process (round-robin). **/

	for(pid = current_pid + 1;
	    pid != current_pid;
	    pid = (pid + 1) % NR_PROC)
	{
		if(proc_tab[pid].used)
		{
			if(proc_tab[pid].state == PROC_SUSPENDED
			&& proc_tab[pid].sigset)
			{
				proc_tab[pid].state = PROC_READY;
				//printk("desuspending %x\n", pid);
			}

			if(proc_tab[pid].state == PROC_READY)
			{
				break;
			}
		}
	}

	/** Switch to the process. **/

	schedule_switch(pid);
}

/**
 * schedule_save_regs
 *
 * NOTE: ebp_isr must be the value of EBP in an C interrupt service routine.
 */

void schedule_save_regs(struct registers *regs, ui32_t *ebp_isr, bool_t exc)
{
	regs->gs = ebp_isr[2];
	regs->fs = ebp_isr[3];
	regs->es = ebp_isr[4];
	regs->ds = ebp_isr[5];
	regs->edi = ebp_isr[6];
	regs->esi = ebp_isr[7];
	regs->ebp = ebp_isr[8];
	regs->ebx = ebp_isr[10];
	regs->edx = ebp_isr[11];
	regs->ecx = ebp_isr[12];
	regs->eax = ebp_isr[13];

	if(exc)
	{
		ebp_isr++;
	}

	regs->eip = ebp_isr[14];
	regs->cs = ebp_isr[15];
	regs->eflags = ebp_isr[16];

	if(ebp_isr[15] == 0x08)
	{
		regs->esp = ebp_isr[9] + 12;
		regs->ss = 0x18;
		if(regs->esp >= 0xf0000000)
		{
			panic("bad saved esp %x (current process: %x)",
			       regs->esp, current_pid);
		}
	}
	else
	{
		regs->esp = ebp_isr[17];
		regs->ss = ebp_isr[18];
	}
}

/**
 * schedule_switch
 */

/** WARNING: The stack this function runs on MUST be different from the kernel
    stack of process pid. Otherwise the registers schedule_switch will push
    on the kernel stack of pid (after changing SS and ESP) may erase local
    variables, which can lead to unexpected results. **/

void schedule_switch(pid_t pid)
{
	ui32_t kesp;
	ui32_t sig;

	if(pid >= NR_PROC)
	{
		panic("invalid pid %x", pid);
	}

	/** Change current_pid **/

	current_pid = pid;
	current = &proc_tab[pid];

	/** Update TSS. **/

	default_tss.esp0 = current->esp0;

	/** Handle first signal if the process we want to switch to is not the
	    root process and if it is currently in user mode. **/

	#ifdef INTERRUPTIBLE_SYSCALLS
	if(current_pid && (current->interruptible || current->regs.cs != 0x08))
	#else
	if(current_pid && current->regs.cs != 0x08)
	#endif
	{
		sig = signal_dequeue(current->sigset, current->sigmask);

		if(sig)
		{
			//printk("handling sig %x for %x\n", sig, current_pid);

			/*if(current->regs.cs == 0x08)
			{
				printk("signal in kernel mode\n");
			}*/

			signal_handle(sig);
		}
	}
	#ifdef DEBUG_SCHEDULE
	else if(current->sigset)
	{
		ui32_t sig;

		for(sig = 0; sig <= 22; sig++)
		{
			if(current->sigset & (1 << (sig - 1)))
			{
				break;
			}
		}

		/*printk("have signal but can't handle sig %x\n", sig);
		printk(">> interruptible: %x | cs2: %x | cs: %x\n",
		       current->interruptible,
		       current->regs2.cs,
		       current->regs.cs);*/
	}
	#endif

	/** Find kernel stack pointer. **/

	if(current->regs.cs == 0x08)
	{
		if(current->regs.esp >= 0xf0000000)
		{
			panic("invalid esp %x (process %x)",
			      current->regs.esp, pid);
		}

		kesp = current->regs.esp;
	}
	else
	{
		kesp = current->esp0;
	}

	/** Do the actual switch. **/

	asm volatile("mov $0x18, %%ax \n\
	              mov %%ax, %%ss \n\
	              mov %0, %%esp \n\
	              mov %1, %%esi \n\
	              mov 0x30(%%esi), %%eax \n\
	              cmp $0x08, %%eax \n\
	              je next \n\
	              pushl 0x3c(%%esi) \n\
	              pushl 0x38(%%esi) \n\
	              next: \n\
	              pushl 0x34(%%esi) \n\
	              pushl 0x30(%%esi) \n\
	              pushl 0x2c(%%esi) \n\
	              pushl 0x28(%%esi) \n\
	              pushl 0x24(%%esi) \n\
	              pushl 0x20(%%esi) \n\
	              pushl 0x1c(%%esi) \n\
	              pushl 0x18(%%esi) \n\
	              pushl 0x14(%%esi) \n\
	              pushl 0x10(%%esi) \n\
	              pushl 0xc(%%esi) \n\
	              pushl 0x8(%%esi) \n\
	              pushl 0x4(%%esi) \n\
	              pushl (%%esi) \n\
	              cli \n\
	              mov $0x20, %%al \n\
	              out %%al, $0x20 \n\
	              mov 0x40(%%esi), %%eax \n\
	              mov %%eax, %%cr3 \n\
	              popl %%gs \n\
	              popl %%fs \n\
	              popl %%es \n\
	              popl %%ds \n\
	              popl %%edi \n\
	              popl %%esi \n\
	              popl %%ebp \n\
	              popl %%ebx \n\
	              popl %%edx \n\
	              popl %%ecx \n\
	              popl %%eax \n\
	              iretl" :: "m"(kesp), "m"(current));
}
