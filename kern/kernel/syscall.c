/****************************************************************
 * syscall.c                                                    *
 *                                                              *
 *    System calls handling.                                    *
 *                                                              *
 ****************************************************************/

#define _SYSCALL_C_
#include <config.h>
#include <fs/cache.h>
#include <fs/ext2.h>
#include <fs/file.h>
#include <kernel/libc.h> // debug
#include <kernel/isr.h>
#include <kernel/panic.h>
#include <kernel/printk.h> // debug
#include <kernel/process.h>
#include <kernel/schedule.h>
#ifdef ENABLE_NETWORK
#include <net/arp.h>
#include <net/ether.h>
#include <net/ip.h>
#endif

#include "syscall.h"

void _isr_syscall()
{
	ui32_t sysc_num;
	ui32_t *param;
	ui32_t ret;
	ui32_t *ebp;

	asm volatile("mov %%eax, %0 \n\
	              mov %%ebx, %3 \n\
	              mov %%esi, %1 \n\
	              mov %%ebp, %%eax \n\
	              mov %%eax, %2" : "=m"(sysc_num), "=m"(param), "=m"(ebp),
	                               "=m"(current->perrno)
	                             :
	                             : "eax", "memory");

	/** Sanity checks. **/

	if(ebp[15] == 0x08)
	{
		panic("syscall in kernel mode (pid: %x)", current_pid);
	}

	if(current->interruptible)
	{
		panic("syscall while interrutible (pid: %x)", current_pid);
	}

	#ifdef DEBUG_SYSCALLS
	printk("beginning of syscall %x\n", sysc_num);
	current->sysc_num = sysc_num;
	#endif

	/** Check errno pointer. **/

	if((current->perrno < (ui32_t*)USER_BASE
	    || current->perrno >= (ui32_t*)USER_STACK_BASE)
	&& current->perrno != &dummy_errno
	&& sysc_num != SYSCALL_SIGRET) // WARNING
	{
		panic("bad errno pointer %x", current->perrno);
	}

	/** Backup the user context as it was before making the system (i.e
	    before int 0x80 instruction). It is used when interrupting a system
	    call. **/

	schedule_save_regs(&current->regs2, ebp, 0);

	if(current->regs2.cs == 0x08)
	{
		panic("syscall from kernel space");
	}

	/** Test sysc_num and try to perform the system call. **/

	cli;

	switch(sysc_num)
	{
		case SYSCALL_PRINT:
			printk((char*)param);
			break;

		case SYSCALL_PRINT_NUMBER:
			printk("%x", *param);
			break;

		case SYSCALL_FORK:
			ret = (ui32_t)sys_fork();
			break;

		case SYSCALL_SIGRET:
			sys_sigret();
			panic("returned from sys_sigret()");
			break;

		case SYSCALL_KILL:
			ret = (ui32_t)sys_kill((pid_t)param[0], param[1]);
			break;

		case SYSCALL_SIGACTION:
			ret = (ui32_t)sys_sigaction(
			              param[0],
			              (struct sigaction*)param[1],
			              (struct sigaction*)param[2]);
			break;

		case SYSCALL_EXIT:
			//printk("exit with status %x\n", *param);
			sys_exit((*param & 0xff) << 8);
			schedule();
			panic("returned from sys_exit()");
			break;

		case SYSCALL_GETPGID:
			{
				pid_t pid = (pid_t)*param;

				if(!pid)
				{
					pid = current_pid;
				}

				if(proc_tab[pid].used)
				{
					ret = (ui32_t)proc_tab[pid].pgid;
					#ifdef DEBUG
					printk("got pgid %x\n", ret);
					#endif
				}
				else
				{
					ret = (ui32_t)-1;
				}
			}
			break;

		case SYSCALL_EXECVE:
			ret = sys_execve((uchar_t*)param[0],
			                 (uchar_t**)param[1],
			                 (uchar_t**)param[2]);
			break;

		case SYSCALL_GETPID:
			ret = (ui32_t)current_pid;
			break;

		case SYSCALL_SETPGID:
			{
				pid_t pid = (pid_t)param[0],
				      pgid = (pid_t)param[1],
				      epid, // effective pid
				      epgid; // effective pgid

				if(!pid)
				{
					epid = current_pid;
				}
				else
				{
					epid = pid;
				}

				if(!pgid)
				{
					epgid = current_pid;
				}
				else
				{
					epgid = pgid;
				}

				if(epid >= NR_PROC || !proc_tab[epid].used)
				{
					ret = (ui32_t)-1;
				}
				else
				{
					proc_tab[epid].pgid = epgid;
					ret = 0;
				}
			}
			break;

		case SYSCALL_SIGPROCMASK:
			ret = (ui32_t)sys_sigprocmask(param[0],
			                              (sigset_t*)param[1],
			                              (sigset_t*)param[2]);
			break;

		case SYSCALL_KILLPG:
			ret = (ui32_t)sys_killpg((pid_t)param[0], param[1]);
			break;

		case SYSCALL_GETPPID:
			ret = (ui32_t)current->parent->pid;
			break;

		case SYSCALL_SIGSUSPEND:
			ret = (ui32_t)sys_sigsuspend((sigset_t*)param);
			break;

		case SYSCALL_SBRK:
			{
				ret = (ui32_t)current->e_heap;
				current->e_heap += *param;
			}
			break;

		case SYSCALL_WAIT4:
			ret = (ui32_t)sys_wait4((pid_t)param[0],
			                        (int*)param[1],
			                        (int)param[2],
			                        (struct rusage*)param[3]);
			break;

		case SYSCALL_OPEN:
			ret = (ui32_t)sys_open((uchar_t*)param[0], param[1]);
			break;

		case SYSCALL_CLOSE:
			ret = (ui32_t)sys_close(*param);
			break;

		case SYSCALL_READ:
			current->interruptible = 1;
			ret = (ui32_t)sys_read(param[0],
			                       (ui8_t*)param[1],
			                       (size_t)param[2]);
			current->interruptible = 0;
			break;

		case SYSCALL_WRITE:
			ret = (ui32_t)sys_write(param[0],
			                        (ui8_t*)param[1],
			                        (size_t)param[2]);
			break;

		case SYSCALL_LSEEK:
			ret = (ui32_t)sys_lseek(param[0],
			                        (off_t)param[1],
			                        param[2]);
			break;

		case SYSCALL_LINK:
			ret = (ui32_t)sys_link((uchar_t*)param[0],
			                       (uchar_t*)param[1]);
			break;

		case SYSCALL_UNLINK:
			ret = (ui32_t)sys_unlink((uchar_t*)param);
			break;

		case SYSCALL_FSTAT:
			ret = (ui32_t)sys_fstat((ui32_t)param[0],
			                        (struct stat*)param[1]);
			break;

		case SYSCALL_ISATTY:
			ret = (ui32_t)sys_isatty(*param);
			break;

		case SYSCALL_FCNTL:
			ret = sys_fcntl(param[0], param[1], (va_list)param[2]);
			break;

		case SYSCALL_CHDIR:
			ret = (ui32_t)sys_chdir((uchar_t*)param);
			break;

		case SYSCALL_GETCWD:
			ret = (ui32_t)sys_getcwd((uchar_t*)param[0],
			                         (size_t)param[1]);
			break;

		case SYSCALL_DUP2:
			ret = (ui32_t)sys_dup2(param[0], param[1]);
			break;

		case SYSCALL_MKDIR:
			ret = (ui32_t)sys_mkdir((uchar_t*)param[0],
			                        (mode_t)param[1]);
			break;

		case SYSCALL_RMDIR:
			ret = (ui32_t)sys_rmdir((uchar_t*)param);
			break;

		case SYSCALL_SYNC:
			{
				if(ext2_sync() != OK)
				{
					panic("failed synchronizing fs");
				}

				if(cache_sync() != OK)
				{
					panic("failed synchronizing disk");
				}
			}
			break;

		case SYSCALL_SELECT:
			current->interruptible = 1;
			ret = (ui32_t)sys_select((si32_t)param[0],
			                         (fd_set*)param[1],
			                         (fd_set*)param[2],
			                         (fd_set*)param[3],
			                         (void*)param[4]);
			current->interruptible = 0;
			break;

		case SYSCALL_GETDTABLESIZE:
			ret = NR_FILDES_PER_PROC;
			break;

		#ifdef ENABLE_PIPES
		case SYSCALL_PIPE2:
			ret = (ui32_t)sys_pipe2((si32_t*)param[0], param[1]);
			break;
		#endif

		case SYSCALL_FTRUNCATE:
			ret = (ui32_t)sys_ftruncate((si32_t)param[0],
			                            (off_t)param[1]);
			break;

		case SYSCALL_FSTATFS:
			ret = (ui32_t)sys_fstatfs((si32_t)param[0],
			                          (struct statfs*)param[1]);
			break;

		case SYSCALL_TCGETATTR:
			ret = (ui32_t)sys_tcgetattr(param[0],
			                            (struct termios*)param[1]);
			break;

		case SYSCALL_TCSETATTR:
			ret = (ui32_t)sys_tcsetattr(param[0],
			                            param[1],
			                            (struct termios*)param[2]);
			break;

		case SYSCALL_TCFLUSH:
			ret = (ui32_t)sys_tcflush(param[0], param[1]);
			break;

		case SYSCALL_TCGETPGRP:
			ret = (ui32_t)sys_tcgetpgrp(*param);
			break;

		case SYSCALL_TCSETPGRP:
			ret = (ui32_t)sys_tcsetpgrp(param[0], (pid_t)param[1]);
			break;

		#ifdef ENABLE_NETWORK
		case SYSCALL_SOCKET:
			ret = (ui32_t)sys_socket(param[0], param[1], param[2]);
			break;

		case SYSCALL_CONNECT:
			ret = (ui32_t)sys_connect((si32_t)param[0],
			                          (struct sockaddr*)param[1],
			                          (socklen_t)param[2]);
			break;

		case SYSCALL_BIND:
			ret = (ui32_t)sys_bind((si32_t)param[0],
			                       (struct sockaddr*)param[1],
			                       (socklen_t)param[2]);
			break;

		case SYSCALL_LISTEN:
			ret = (ui32_t)sys_listen((si32_t)param[0], param[1]);
			break;

		case SYSCALL_ACCEPT:
			current->interruptible = 1;
			ret = (ui32_t)sys_accept((si32_t)param[0],
			                         (struct sockaddr*)param[1],
			                         (socklen_t*)param[2]);
			current->interruptible = 0;
			break;

		case SYSCALL_RECV:
			current->interruptible = 1;
			ret = (ui32_t)sys_recv((si32_t)param[0],
			                       (void*)param[1],
			                       (size_t)param[2],
			                       param[3]);
			current->interruptible = 0;
			break;

		case SYSCALL_SEND:
			ret = (ui32_t)sys_send((si32_t)param[0],
			                       (void*)param[1],
			                       (size_t)param[2],
			                       param[3]);
			break;

		case SYSCALL_GETSOCKOPT:
			ret = (ui32_t)sys_getsockopt((si32_t)param[0],
			                             param[1],
			                             param[2],
			                             (void*)param[3],
			                             (socklen_t*)param[4]);
			break;

		case SYSCALL_SETSOCKOPT:
			ret = (ui32_t)sys_setsockopt((si32_t)param[0],
			                             param[1],
			                             param[2],
			                             (void*)param[3],
			                             (socklen_t)param[4]);
			break;

		case SYSCALL_GETSOCKNAME:
			ret = (ui32_t)sys_getsockname((si32_t)param[0],
			                              (void*)param[1],
			                              (void*)param[2]);
			break;
		#endif

		#ifdef ENABLE_NETWORK
		case SYSCALL_NETCONF:
			{
				clock_t old_tics = 0;
				count_t try_cnt = 0;

				current->interruptible = 1;

				local_ip = param[0];
				gw_ip = param[1];

				gw_hwaddr_set = 0;

				do
				{
					sti;
					yield;
					cli;

					if(tics - old_tics > CLK_FREQ / 2)
					{
						old_tics = tics;
						arp_request(gw_ip);
						try_cnt++;
					}
				} while(!gw_hwaddr_set && try_cnt < 10);

				if(gw_hwaddr_set)
				{
					ret = 0;
				}
				else
				{
					ret = (ui32_t)-1;
				}

				current->interruptible = 0;
			}
			break;
		#endif

		default:
			ret = (ui32_t)-1;
			//panic("unsupported syscall %x", sysc_num);
			break;
	}

	sti;

	#ifdef DEBUG_SYSCALLS
	//printk("end of syscall %x\n", sysc_num);
	#endif

	/** Store the return value into the stack (it should go into EAX after
	    iret). **/

	ebp[13] = ret;

	#ifdef EXTENDED_SIGNAL_HANDLING

	/** Handle signals if the current process is not the root process and
	    if it is currently in user mode. **/

	cli;

	//printk("handling signals from syscall\n");

	if(current_pid/* && ebp[15] != 0x08*/)
	{
		/** Update the registers of the process structure (indeed,
		    schedule_switch() relies on them). WARNING: Do not copy
		    current->regs2 into current->regs (the stack may have
		    been modified while performing syscall). */

		schedule_save_regs(&current->regs, ebp, 0);

		/** Changing the stack is necessary, because schedule_switch()
		    uses the kernel stack of the process we want to switch
		    to... which is the current process. In fact,
		    schedule_switch() uses the last saved value of the kernel
		    stack pointer to determine where the kernel stack is. And
		    as we are currently operating on the kernel stack of the
		    current process, the kernel stack pointer value may have
		    evolved since the system switched to the process we are
		    in. **/

		asm volatile("mov $0x6ffc, %%esp" ::: "esp");

		schedule_switch(current_pid);

		panic("failed returning from syscall\n");
	}

	sti;

	#endif
}
