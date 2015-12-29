/****************************************************************
 * exit.c                                                       *
 *                                                              *
 *    exit syscall.                                             *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/tty.h>
#include <kernel/panic.h>
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/signal.h>
#include <mm/mem_map.h>
#include <mm/paging.h>
#ifdef ENABLE_NETWORK
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <net/tcp.h>
#endif

/**
 * sys_exit
 */

void sys_exit(int status)
{
	struct process *root = &proc_tab[0],
	               *son;
	si32_t fildes;

	/** Check whether we can exit from the process. **/

	if(current_pid == 0)
	{
		panic("root process is trying to exit");
	}
	else if(!current->parent)
	{
		panic("process has no parent");
	}

	/** If the current process has children, have them "adopted" by the
	    root process. **/

	if(current->first_son)
	{
		if(!root->first_son)
		{
			panic("root process has no son");
		}

		/** Update the parent of each son. **/

		son = current->first_son;

		do
		{
			son->parent = root;
			son = son->next_sibling;
		} while(son != current->first_son);

		/** Now, merge the list of sons of the root process with that of
		    the current process. **/
	
		root->first_son->next_sibling->prev_sibling
		 = current->first_son->prev_sibling;
		current->first_son->prev_sibling->next_sibling
		 = root->first_son->next_sibling;
		root->first_son->next_sibling = current->first_son;
		current->first_son->prev_sibling = root->first_son;
	}

	/** Close every opened file. **/

	for(fildes = 0; fildes < NR_FILDES_PER_PROC; fildes++)
	{
		if(current->pfildes_tab[fildes])
		{
			fildes_unref(fildes);
		}
	}

	/** Signal the death of the process to the parent. **/

	current->parent->dead_son_pid = current_pid;
	current->parent->dead_son_status = status;
	current->parent->sigset |= (1 << (SIGCHLD - 1));
	current->state = PROC_ZOMBIE;
	current->status = status;
	current->reported = 0;

	/** Do not forget to update nproc. **/

	nproc--;

	/** If the process was trying to read from the terminal, free the
	    terminal. **/

	if(reader_pid == current_pid)
	{
		reader_pid = 0;
	}

	/** As the kernel stack we are currently using is going to be freed and
	    unmapped from virtual memory, we need to switch to a default kernel
	    stack. **/

	asm volatile("mov $0x6ffc, %%esp \n\
	              mov $0x18, %%ax \n\
	              mov %%ax, %%ss" ::: "eax", "esp");

	paging_vfree(current->kstack_page);

	/** Free user pages. **/

	asm volatile("mov %0, %%eax \n\
	              mov %%eax, %%cr3" :
	                                : "i"(KERNEL_PD_BASE)
	                                : "eax");

	paging_destroy_pd(current->pd);

	/** Switch to the first process. **/

	schedule_switch(0);
}
