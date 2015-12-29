/****************************************************************
 * sigret.c                                                     *
 *                                                              *
 *    sigret syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/schedule.h>

void sys_sigret()
{
	ui32_t *ebp_isr, *esp;

	asm volatile("mov (%%ebp), %%eax \n\
	              mov %%eax, %0" : "=m"(ebp_isr)
	                             :
	                             : "eax", "memory");

	/*if(ebp_isr[15] == 0x08)
	{
		panic("trying to return from signal in kernel mode");
	}*/

	/** Get the value the stack pointer had just after returning from the
	    signal handler. **/

	if(ebp_isr[15] == 0x08)
	{
		esp = (ui32_t*)(ebp_isr[9] + 12);
	}
	else
	{
		esp = (ui32_t*)ebp_isr[17];
	}

	/** Restore the registers and the signal mask. **/

	current->regs.gs = esp[3];
	current->regs.fs = esp[4];
	current->regs.es = esp[5];
	current->regs.ds = esp[6];
	current->regs.edi = esp[7];
	current->regs.esi = esp[8];
	current->regs.ebp = esp[9];
	current->regs.ebx = esp[10];
	current->regs.edx = esp[11];
	current->regs.ecx = esp[12];
	current->regs.eax = esp[13];
	current->regs.eip = esp[14];
	current->regs.cs = esp[15];
	current->regs.eflags = esp[16];
	current->regs.esp = esp[17];
	current->regs.ss = esp[18];
	current->sigmask = esp[19];

	schedule_switch(0);
}
