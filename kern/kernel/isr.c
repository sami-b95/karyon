/****************************************************************
 * isr.c                                                        *
 *                                                              *
 *    Interrupt service routines for exceptions and IRQs.       *
 *                                                              *
 ****************************************************************/

#define _ISR_C_
#include <config.h>
#include <fs/fifo.h>
#include <fs/tty.h>
#include <fs/vt100.h> // debug
#include <kernel/errno.h>
#include <kernel/io.h>
#include <kernel/kbdmap.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/screen.h>
#include <kernel/signal.h>
#include <kernel/syscall.h>
#include <mm/mem_map.h>
#include <mm/paging.h>
#ifdef ENABLE_NETWORK
#include <net/rtl8139.h>
#include <net/tcp.h>
#endif

#define _ISR_C_
#include "isr.h"

/**
 * _isr_default_exc
 */

void _isr_default_exc()
{
	#ifndef PANIC_ON_EXC
	ui32_t *ebp;
	#endif

	//dump_stack();
	#ifdef PANIC_ON_EXC
	panic("exception");
	#else
	cli;
	asm volatile("mov %%ebp, %0" : "=m"(ebp));	
	schedule_save_regs(&current->regs, ebp, 1);
	current->sigset |= (1 << (SIGKILL - 1));
	schedule_switch(current_pid);
	//sys_kill(current_pid, SIGQUIT);
	#endif
}

/**
 * _isr_pf_exc
 */

void _isr_pf_exc()
{
	ui32_t error_code;
	void *bad_vaddr, *bad_vpage_base; ui32_t bad_vpage;
	ui32_t ppage;
	/** This buffer is protected from race conditions because interrupts
	    are disabled when handling a page fault. **/
	static ui8_t page_buf[4096];
	#ifndef PANIC_ON_EXC
	ui32_t *ebp;
	#endif

	//dump_stack();
	asm volatile("mov %%cr2, %%eax \n\
	              mov %%eax, %0 \n\
	              mov 0x38(%%ebp), %%eax \n\
	              mov %%eax, %1" : "=m"(bad_vaddr), "=m"(error_code)
	                             :
	                             : "eax", "memory");

	#ifdef DEBUG
	printk("page fault @%x, error_code: %x\n", bad_vaddr, error_code);
	#endif

	/*if(ebp[15] == 0x08)
	{
		panic("page fault in kernel mode");
	}*/

	if((ui32_t)bad_vaddr < USER_BASE || (ui32_t)bad_vaddr >= RESERVED_BASE
	#ifdef TOUCHY_BAD_PF
	|| (bad_vaddr < (void*)0xe0000000 && bad_vaddr > (void*)0x50000000)
	#endif
	)
	{
		if(!(error_code & EXC_PF_USER))
		{
			/** The page fault occured in kernel mode. **/

			panic("page fault from kernel mode (bad vaddr: %x)",
			      bad_vaddr);
		}

		/** The page fault occured in user mode. **/

		#ifdef PANIC_ON_EXC
		panic("bad page fault (process %x, bad vaddr: %x, error: %x)",
		      current_pid, bad_vaddr, error_code);
		#else
		cli;
		asm volatile("mov %%ebp, %0" : "=m"(ebp));	
		schedule_save_regs(&current->regs, ebp, 1);
		current->sigset |= (1 << (SIGSEGV - 1));
		schedule_switch(current_pid);
		#endif
	}
	else if(error_code > 7)
	{
		panic("invalid error code %x", error_code);
	}

	bad_vpage = (ui32_t)bad_vaddr >> 12;

	if((error_code & EXC_PF_PRESENT) && (error_code && EXC_PF_WRITE))
	{
		/** Copy-on-write **/

		#ifdef DEBUG
		printk("copy on write\n");
		#endif

		ppage = paging_palloc();

		if(!ppage)
		{
			panic("no physical memory left");
		}

		bad_vpage_base = (void*)(bad_vpage << 12);
		memcpy(page_buf, bad_vpage_base, 4096);

		/*** After unmapping, the physical page mapped to bad_vpage is
		     freed automatically by paging_unmap if its reference
		     counter becomes null: this is possible because bad_vpage
		     is a user page. ***/

		paging_unmap(bad_vpage);

		if(paging_map(ppage, bad_vpage) != OK)
		{
			panic("no physical memory left (could not map page)");
		}

		memcpy(bad_vpage_base, page_buf, 4096);
	}
	else if(error_code & EXC_PF_PRESENT)
	{
		panic("unusual page fault exception");
	}
	else
	{
		#ifdef DEMAND_PAGING
		#ifdef DEBUG
		printk("demand paging\n");
		#endif

		ppage = paging_palloc();

		if(!ppage)
		{
			panic("no physical memory left");
		}

		if(paging_map(ppage, bad_vpage) != OK)
		{
			panic("no physical memory left (could not map page)");
		}

		#ifdef PAGING_ZERO
		memset((void*)(bad_vpage << 12), 0, 4096);
		#endif

		#else
		panic("demand paging not supported");
		#endif
	}
}

/**
 * _isr_default_pic1_irq
 */

void _isr_default_pic1_irq()
{
	#ifdef DEBUG
	printk("pic1 irq\n");
	#endif
}

/**
 * _isr_default_pic2_irq
 */

void _isr_default_pic2_irq()
{
	#ifdef DEBUG
	printk("pic2 irq\n");
	#endif
}

/**
 * _isr_pit_irq
 */

void _isr_pit_irq()
{
	tics++;

	#ifdef ENABLE_NETWORK
	tcp_callback();
	#endif

	schedule();
}

/**
 * _isr_kbd_irq
 */

void _isr_kbd_irq()
{
	ui8_t scancode;
	uchar_t *s;
	static bool_t lshift_enabled = 0,
	              rshift_enabled = 0,
	              ctrl_enabled = 0,
	              alt_enabled = 0;

	scancode = inb(KEYBOARD_DATA_PORT);

	if(scancode <= 0x80)
	{
		if(scancode == KEYBOARD_LEFT_SHIFT_SCANCODE)
		{
			lshift_enabled = 1;
		}
		else if(scancode == KEYBOARD_RIGHT_SHIFT_SCANCODE)
		{
			rshift_enabled = 1;
		}
		else if(scancode == KEYBOARD_CTRL_SCANCODE)
		{
			ctrl_enabled = 1;
		}
		else if(scancode == KEYBOARD_ALT_SCANCODE)
		{
			alt_enabled = 1;
		}
		else if(scancode == KEYBOARD_F1_SCANCODE)
		{
			fifo_flush(&tty_ififo);
		}
		else if(scancode == KEYBOARD_F1_SCANCODE + 1)
		{
			fifo_flush(&tty_ififo2);
		}
		else if(scancode == KEYBOARD_F1_SCANCODE + 2)
		{
			printk("memory left: %x\n", ppage_left << 12);
		}
		else if(scancode == KEYBOARD_F1_SCANCODE + 3)
		{
			show_cursor ^= 1;
		}
		else if(scancode == KEYBOARD_F1_SCANCODE + 4)
		{
			vt100_scroll_up();
		}
		else if(scancode == KEYBOARD_F1_SCANCODE + 5)
		{
			vt100_scroll_down();
		}
		#ifdef ENABLE_NETWORK
		else if(scancode == KEYBOARD_F1_SCANCODE + 6)
		{
			rtl8139_reset();
		}
		#endif
		else if(scancode == KEYBOARD_F1_SCANCODE + 7)
		{
			asm volatile("cli \n\
			              hlt");
		}
		else if(scancode == KEYBOARD_F1_SCANCODE + 7)
		{
			struct process *root = &proc_tab[0];
			struct process *proc;

			if(root->first_son)
			{
				proc = root->first_son;

				do
				{
					printk("%x\n", proc->pid);
					proc = proc->prev_sibling;
				} while(proc != root->first_son);
			}
		}
		else if(ctrl_enabled && *kbd_map[scancode * 4 + 1] >= 0x40)
		{
			uchar_t c = *kbd_map[scancode * 4 + 1] - 0x40;

			if(tty_write(&c, 1) != OK)
			{
				screen_print("tty input failed\n");
			}

			/*if(tty_process() != OK)
			{
				screen_print("tty char processing failed\n");
			}*/
		}
		else
		{
			s = kbd_map[(lshift_enabled || rshift_enabled)
			            + scancode * 4];

			if(tty_write(s, strlen(s)) != OK)
			{
				screen_print("tty input failed\n");
			}

			/*if(tty_process() != OK)
			{
				screen_print("tty char processing failed\n");
			}*/
		}

		//screen_show_cursor();
	}
	else
	{
		scancode -= 0x80;

		if(scancode == KEYBOARD_LEFT_SHIFT_SCANCODE)
		{
			lshift_enabled = 0;
		}
		else if(scancode == KEYBOARD_RIGHT_SHIFT_SCANCODE)
		{
			rshift_enabled = 0;
		}
		else if(scancode == KEYBOARD_CTRL_SCANCODE)
		{
			ctrl_enabled = 0;
		}
		else if(scancode == KEYBOARD_ALT_SCANCODE)
		{
			alt_enabled = 0;
		}
	}
}

/**
 * _isr_0x09_irq
 */

void _isr_0x09_irq()
{
	#ifdef DEBUG
	printk("0x09 irq\n");
	#endif

	if(_isr_0x09_irq_handler)
	{
		_isr_0x09_irq_handler();
	}
}

/**
 * _isr_0x0a_irq
 */

void _isr_0x0a_irq()
{
	#ifdef DEBUG
	printk("0x0a irq\n");
	#endif

	if(_isr_0x0a_irq_handler)
	{
		_isr_0x0a_irq_handler();
	}
}

/**
 * _isr_0x0b_irq
 */

void _isr_0x0b_irq()
{
	#ifdef DEBUG
	printk("0x0b irq\n");
	#endif

	if(_isr_0x0b_irq_handler)
	{
		_isr_0x0b_irq_handler();
	}
}

/**
 * _isr_yield
 */

void _isr_yield()
{
	schedule();
}
