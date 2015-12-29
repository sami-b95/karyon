/****************************************************************
 * fork.c                                                       *
 *                                                              *
 *    fork syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/process.h>
#include <kernel/types.h>

/**
 * sys_fork
 */

pid_t sys_fork()
{
	ui32_t *ebp_isr;
	ui32_t *son_pd;
	pid_t son_pid;
	struct process *son;
	si32_t fildes;

	asm volatile("mov (%%ebp), %%eax \n\
	              mov %%eax, %0" : "=m"(ebp_isr)
	                             :
	                             : "eax", "memory");

	/** Initialize copy-on-write. **/

	son_pd = paging_cow_init();

	if(!son_pd)
	{
		goto fail2;
	}

	/** Create a process. **/

	son_pid = process_create(current, 0, 0);

	if(son_pid == -1)
	{
		goto fail1;
	}

	son = &proc_tab[son_pid];

	/** Copy registers (except EAX, which must be set to 0). **/

	son->regs.gs = ebp_isr[2];
	son->regs.fs = ebp_isr[3];
	son->regs.es = ebp_isr[4];
	son->regs.ds = ebp_isr[5];
	son->regs.edi = ebp_isr[6];
	son->regs.esi = ebp_isr[7];
	son->regs.ebp = ebp_isr[8];
	son->regs.ebx = ebp_isr[10];
	son->regs.edx = ebp_isr[11];
	son->regs.ecx = ebp_isr[12];
	son->regs.eax = 0;
	son->regs.eip = ebp_isr[14];
	son->regs.cs = ebp_isr[15];
	son->regs.eflags = ebp_isr[16];

	if(ebp_isr[15] == 0x08)
	{
		panic("fork not allowed in kernel mode");
	}
	else
	{
		son->regs.esp = ebp_isr[17];
		son->regs.ss = ebp_isr[18];
	}

	/** Copy some other fields. **/

		/** IDs. **/

	son->pgid = current->pgid;

		/** Memory. **/

	son->b_heap = current->b_heap;
	son->e_heap = current->e_heap;

		/** File descriptors and current working directory. **/

	file_unref(1);
	file_unref(1);
	file_unref(1);

	if(fildes_tab[0].ref_cnt < 4)
	{
		panic("tty referenced less than 4 times");
	}

	fildes_tab[0].ref_cnt -= 3;

	for(fildes = 0; fildes < NR_FILDES_PER_PROC; fildes++)
	{
		if(current->pfildes_tab[fildes])
		{
			/*fildes_copy(&son->fildes_tab[fildes],
			            &current->fildes_tab[fildes]);*/
			son->pfildes_tab[fildes]
			 = current->pfildes_tab[fildes];
			son->fildes_flags[fildes]
			= current->fildes_flags[fildes];
			fildes_ref(fildes);
		}
	}

	strncpy(son->cwd, current->cwd, PATH_MAX_LEN + 1);

		/** Signals. **/

	son->sigset = current->sigset;
	son->sigmask = current->sigmask;
	/*memcpy(son->sigact, current->sigact, sizeof(struct sigaction) * 31);*/

		/** Pointer to errno. FIXME: Really useful? **/

	son->perrno = current->perrno;

	/** Update the page directory of the son. **/

	paging_destroy_pd(son->pd);
	son->pd = son_pd;

	return son_pid;

	fail1:
		paging_destroy_pd(son_pd);
	fail2:
		return -1;
}
