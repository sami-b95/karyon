/****************************************************************
 * process.c                                                    *
 *                                                              *
 *    Process manager.                                          *
 *                                                              *
 ****************************************************************/

#define _PROCESS_C_
#include <config.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <mm/paging.h>

#include "process.h"

/**
 * process_create
 */

pid_t process_create(struct process *parent, void *code, size_t code_size)
{
	pid_t pid;
	count_t code_page_count;
	ui32_t code_ppage, code_vpage,
	       ustack_ppage, ustack_vpage;
	struct pheap_page kstack_page;
	si32_t fildes;
	/*struct fildes def_io = {
		.used = 1,
		.flags = O_RDWR | O_APPEND,
		.off = 0,
		.inum = 1
	};*/
	ui32_t sig;
	ui32_t old_cr3;

	if(nproc == NR_PROC)
	{
		goto fail2;
	}

	/** Find a free pid. **/

	for(pid = 0; pid < NR_PROC; pid++)
	{
		if(!proc_tab[pid].used)
		{
			break;
		}
	}

	if(pid == NR_PROC)
	{
		panic("process table corrupted");
	}

	/** Create a page directory. **/

	proc_tab[pid].pd = paging_create_pd();

	if(!proc_tab[pid].pd)
	{
		goto fail2;
	}

	/** Switch to this page directory. **/

	asm volatile("mov %%cr3, %%eax \n\
	              mov %%eax, %0 \n\
	              mov %1, %%eax \n\
	              mov %%eax, %%cr3" : "=m"(old_cr3)
	                                : "m"(proc_tab[pid].pd)
	                                : "eax", "memory");

	/** Allocate and map pages for code. **/

	code_page_count = (code_size + 4095) >> 12;

		// WARNING: immediate value.

	for(code_vpage = 0x40000;
	    code_vpage < (0x40000 + code_page_count);
	    code_vpage++)
	{
		code_ppage = paging_palloc();

		if(!code_ppage)
		{
			goto fail1;
		}

		if(paging_map(code_ppage, code_vpage) != OK)
		{
			paging_pfree(code_ppage);
			goto fail1;
		}
	}

	/** Allocate and map a page for user stack. **/

	ustack_ppage = paging_palloc();

	if(!ustack_ppage)
	{
		goto fail1;
	}

	ustack_vpage = (ui32_t)(USER_STACK_BASE >> 12) - 1;

	if(paging_map(ustack_ppage, ustack_vpage) != OK)
	{
		paging_pfree(ustack_ppage);
		goto fail1;
	}

	/** Allocate and map a page for kernel stack. **/

	kstack_page = paging_valloc(1);

	if(!kstack_page.vpage)
	{
		goto fail1;
	}

	/** Copy the code. **/

	memcpy((void*)USER_BASE, code, code_size);

	/** Initialize registers. **/

	proc_tab[pid].regs.gs
	= proc_tab[pid].regs.fs
	= proc_tab[pid].regs.es
	= proc_tab[pid].regs.ds = 0x2b;
	proc_tab[pid].regs.edi = proc_tab[pid].regs.esi = 0;
	proc_tab[pid].regs.ebp = 0;
	proc_tab[pid].regs.ebx
	= proc_tab[pid].regs.edx
	= proc_tab[pid].regs.ecx
	= proc_tab[pid].regs.eax = 0;
	proc_tab[pid].regs.eip = (ui32_t)USER_BASE;
	proc_tab[pid].regs.cs = 0x23;
	proc_tab[pid].regs.eflags = 0x202;
	proc_tab[pid].regs.esp = (ui32_t)USER_STACK_BASE - 4;
	proc_tab[pid].regs.ss = 0x33;

	/** Initialize other fields. **/

		/** IDs. **/

	proc_tab[pid].pid = pid;
	proc_tab[pid].pgid = pid;

		/** Memory. **/

	proc_tab[pid].used = 1;
	proc_tab[pid].kstack_page = kstack_page;
	proc_tab[pid].esp0 = (kstack_page.vpage << 12) + 4092;
	proc_tab[pid].b_heap = (void*)(((ui32_t)USER_BASE + code_size + 4)
	                                        & ~3);
	proc_tab[pid].e_heap = proc_tab[pid].b_heap;

		/** Process family. **/

	proc_tab[pid].parent = parent;
	proc_tab[pid].first_son = 0;

		/** File descriptors and current working directory. **/

	for(fildes = 0; fildes < NR_FILDES_PER_PROC; fildes++)
	{
		proc_tab[pid].pfildes_tab[fildes] = 0;
	}

	proc_tab[pid].pfildes_tab[0]
	= proc_tab[pid].pfildes_tab[1]
	= proc_tab[pid].pfildes_tab[2] = &fildes_tab[0];
	proc_tab[pid].fildes_flags[0]
	= proc_tab[pid].fildes_flags[1]
	= proc_tab[pid].fildes_flags[2] = O_RDWR | O_APPEND;
	fildes_tab[0].ref_cnt += 3; // do not use fildes_ref here

	strncpy(proc_tab[pid].cwd, "/", PATH_MAX_LEN + 1);

		/** Signals. **/

	proc_tab[pid].sigset = 0;
	proc_tab[pid].sigmask = 0;
	for(sig = 1; sig < 32; sig++)
	{
		proc_tab[pid].sigact[sig - 1].sa_handler = SIG_DFL;
	}
	proc_tab[pid].interruptible = 0;

		/** State and wait syscall stuff. **/

	proc_tab[pid].state = PROC_READY;
	proc_tab[pid].status = 0;
	proc_tab[pid].reported = 0;

		/** Information for wait. **/

	proc_tab[pid].dead_son_pid = 0;
	proc_tab[pid].dead_son_status = 0;

		/** Pointer to errno (for syscalls). **/

	proc_tab[pid].perrno = 0;

		/** Debugging. **/

	proc_tab[pid].sysc_num = 0;

	/** Set prev_sibling and next_sibling if the process has a parent **/

	if(parent)
	{
		if(!parent->first_son)
		{
			parent->first_son = &proc_tab[pid];
			proc_tab[pid].prev_sibling
			= proc_tab[pid].next_sibling
			= &proc_tab[pid];
		}
		else
		{
			proc_tab[pid].prev_sibling = parent->first_son;
			proc_tab[pid].next_sibling
			 = parent->first_son->next_sibling;
			parent->first_son->next_sibling->prev_sibling
			 = &proc_tab[pid];
			parent->first_son->next_sibling = &proc_tab[pid];
		}
	}

	/** Update nproc. **/

	nproc++;

	/** Reference the tty file three times. **/

	file_ref(1);
	file_ref(1);
	file_ref(1);

	/** Switch back to initial page directory. **/

	asm volatile("mov %0, %%eax \n\
	              mov %%eax, %%cr3" :
	                                : "m"(old_cr3)
	                                : "eax");

	return pid;

	fail1:
		paging_destroy_pd(proc_tab[pid].pd);
		asm volatile("mov %0, %%eax \n\
		              mov %%eax, %%cr3" :
		                                : "m"(old_cr3)
		                                : "eax");
	fail2:
		return -1;
}
