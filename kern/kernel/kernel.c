/****************************************************************
 * kernel.c                                                     *
 *                                                              *
 *    Main kernel function.                                     *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ata.h>
#ifdef USE_CACHE
#include <fs/cache.h>
#endif
#include <fs/ext2.h>
#include <fs/fifo.h> // debug
#include <fs/file.h> // debug
#include <fs/path.h> // debug
#include <kernel/errno.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/int.h>
#include <kernel/io.h> // debug
#include <kernel/isr.h> // debug
#include <kernel/libc.h> // debug
#include <kernel/panic.h>
#include <kernel/pic.h>
#include <kernel/printk.h>
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/screen.h>
#include <kernel/syscall.h> // debug
#include <mm/paging.h>
#ifdef ENABLE_NETWORK
#include <net/endian.h> // debug
#include <net/ether.h> // debug
#include <net/ip.h> // debug
#include <net/rtl8139.h>
#include <net/tcp.h> // debug
#endif

void umain();
void test();

clock_t select_tics = 0;
clock_t waiting_ready_tics = 0;
clock_t io_op_tics = 0;

/**
 * kmain
 */

void kmain()
{
	pid_t pid;

	cli;
	screen_clear();

	printk("init pic...\t");
	pic_init();
	printk("ok\r\n");

	printk("init gdt...\t");
	gdt_init();
	asm volatile("movw $0x38, %%ax \n\
	              ltr %%ax \n\
	              movw $0x18, %%ax \n\
	              movw %%ax, %%ss \n\
	              movl $0x6ffc, %%esp" ::: "eax", "esp");
	printk("ok\r\n");

	printk("init idt...\t");
	idt_init();
	printk("ok\r\n");

	printk("init paging...\t");
	paging_init();
	printk("ok\r\n");

	printk("init ata...\t");
	ata_init(ATA_CTL);
	printk("ok\r\n");

	#ifdef USE_CACHE
	printk("init cache..\t");
	cache_init();
	printk("ok\r\n");
	#endif

	printk("init ext2...\t");
	ext2_init();
	printk("ok\r\n");

	#ifdef ENABLE_NETWORK
	printk("init rtl8139...\t");
	rtl8139_init();
	printk("ok\r\n");
	#endif

	#ifdef ENABLE_FPU
	printk("init fpu...\t");
	asm volatile("finit");
	printk("ok\r\n");
	#endif

	printk("kernel started\r\nmemory: 0x%x/0x%x bytes left\r\n",
	       ppage_left << 12,
	       RAM_SIZE);

	//test();

	pid = process_create(0, umain, 420);

	if(pid == -1)
	{
		panic("failed creating root process");
	}

	schedule_switch(pid);

	panic("failed switching to root process");

	while(1);
}

void sprintf(char *s, char *f, ...)
{
	va_list ap;
	va_start(ap, f);
	vsnprintf(s, 13, f, ap);
	va_end(ap);
}

uchar_t *argv[1] = { 0 };

void umain()
{
	pid_t ret, sh_pid;
	int ret2;
	uchar_t *error1 = "failed forking";
	uchar_t *error2 = "failed loading binary";
	uchar_t *error3 = "shell terminated\r\n";
	uchar_t *error4 = "failed setting pgrp\r\n";
	ui32_t param[3] = { (ui32_t)"/bin/dash", (ui32_t)argv, 0 };
	ui32_t *pparam = param;
	int status;
	ui32_t param2[4] = { -1, (ui32_t)&status, 0, 0 }; 
	ui32_t *pparam2 = param2;
	ui32_t param3[2] = { 0, 1 };
	ui32_t *pparam3 = param3;
	ui32_t param4[3] = { 1, 1 };
	ui32_t *pparam4 = param4;

	launch_shell:

	syscall(SYSCALL_TCSETPGRP, pparam3, ret);

	if(ret)
	{
		syscall_no_ret(SYSCALL_PRINT, error4);
		while(1);
	}

	syscall_no_param(SYSCALL_FORK, ret);

	if(ret == -1)
	{
		syscall_no_ret(SYSCALL_PRINT, error1);
		while(1);
	}
	else if(ret == 0)
	{
		syscall(SYSCALL_SETPGID, pparam4, ret2);
		syscall(SYSCALL_EXECVE, pparam, ret2);
		syscall_no_ret(SYSCALL_PRINT, error2);
		while(1);
	}
	else
	{
		sh_pid = ret;

		while(1)
		{
			syscall(SYSCALL_WAIT4, pparam2, ret);

			if(ret == sh_pid)
			{
				syscall_no_ret(SYSCALL_PRINT, error3);
				goto launch_shell;
			}
		}
	}
}
