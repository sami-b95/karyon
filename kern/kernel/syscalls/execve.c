/****************************************************************
 * execve.c                                                     *
 *                                                              *
 *    execve syscall.                                           *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/ext2.h>
#include <fs/file.h>
#include <fs/path.h>
#include <kernel/elf.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h> // for debugging
#include <kernel/printk.h> // debugging
#include <kernel/process.h>
#include <kernel/schedule.h>
#include <kernel/syscall.h>
#include <kernel/types.h>

/**
 * sys_execve
 */

int sys_execve(uchar_t *bin, uchar_t *argv[], uchar_t *envp[])
{
	ui32_t ext2_inum;
	struct elf32_ehdr ehdr;
	struct elf32_phdr phdr;
	ui32_t ph;
	ui32_t argc, arg;
	ui32_t *argv_base;
	void *argv_str_base;
	size_t argv_str_size;
	off_t argv_str_off;
	ui32_t esp;
	si32_t fildes;
	struct fildes *fd;

	if(!current_pid)
	{
		panic("root process wants to replace itself!");
	}

	/** Try to find the ELF file and to read its main header. **/

	strncpy(tmp_path, bin, PATH_MAX_LEN + 1);

	if(path_clean(tmp_path) != OK)
	{
		return -1;
	}

	if(path_to_file(tmp_path, &ext2_inum, 0, 0) != OK)
	{
		return -1;
	}

	if(ext2_read(ext2_inum, &ehdr, sizeof(struct elf32_ehdr), 0) < 0)
	{
		return -1;
	}

	if(ehdr.e_ident[0] != 0x7f || ehdr.e_ident[1] != 'E'
	|| ehdr.e_ident[2] != 'L' || ehdr.e_ident[3] != 'F')
	{
		return -1;
	}

	/** If any argument is provided, push the arguments to the stack. Do
	    not forget the null string pointer (some programs use it to
	    determine the number of arguments).**/

	if(argv)
	{
		for(argc = 0, argv_str_size = 0;
		    argv[argc];
		    argv_str_size += strlen(argv[argc]) + 1, argc++)
		{
			#ifdef DEBUG
			printk("argv[%x] = %x\n", argc, (ui32_t)argv[argc]);
			#endif
		}

		argv_str_base = (void*)USER_STACK_BASE - argv_str_size;
		argv_base = argv_str_base - (argc + 1) * 4;
		argv_base = (void*)argv_base - ((ui32_t)argv_base & 3);

		for(arg = 0, argv_str_off = 0;
		    arg < argc;
		    argv_str_off += strlen(argv[arg]) + 1, arg++)
		{
			strncpy(argv_str_base + argv_str_off,
			        argv[arg],
			        argv_str_size - argv_str_off);
			argv_base[arg] = (ui32_t)argv_str_base + argv_str_off;
		}

		argv_base[argc] = 0;

		*(argv_base - 1) = 0; // empty envp array
		*(argv_base - 2) = (ui32_t)(argv_base - 1); // envp
		*(argv_base - 3) = (ui32_t)argv_base; // argv
		*(argv_base - 4) = argc; // argc
		esp = (ui32_t)(argv_base - 4);
	}
	else
	{
		*((ui32_t*)(USER_STACK_BASE - 4)) = 0; // empty envp array
		*((ui32_t*)(USER_STACK_BASE - 8))
		 = (ui32_t)(USER_STACK_BASE - 4); // envp
		*((ui32_t*)(USER_STACK_BASE - 12)) = 0; // argv = 0
		*((ui32_t*)(USER_STACK_BASE - 16)) = 0; // argc = 0
		esp = (ui32_t)USER_STACK_BASE - 16;
	}

	/** Load the ELF file. **/

	current->b_heap = 0;

	for(ph = 0; ph < ehdr.e_phnum; ph++)
	{
		if(ext2_read(ext2_inum,
		             &phdr,
		             sizeof(struct elf32_phdr),
		             ehdr.e_phoff
		           + ph * sizeof(struct elf32_phdr)) < 0)
		{
			sys_exit(-1);
		}

		if(phdr.p_type == 1)
		{
			if(phdr.p_vaddr >= USER_BASE
			&& phdr.p_vaddr < USER_STACK_BASE)
			{
				#ifdef DEBUG
				printk("-------------------------------\n");
				printk("\tp_vaddr: %x\n", phdr.p_vaddr);
				printk("\tp_filesz: %x\n", phdr.p_filesz);
				printk("\tp_memsz: %x\n", phdr.p_memsz);
				#endif

				memset((void*)phdr.p_vaddr, 0, phdr.p_memsz);

				if(ext2_read(ext2_inum,
				             (void*)phdr.p_vaddr,
				             phdr.p_filesz,
				             phdr.p_offset) < 0)
				{
					sys_exit(-1);
				}

				if((void*)phdr.p_vaddr + phdr.p_memsz
				    > current->b_heap)
				{
					current->b_heap
					 = (void*)phdr.p_vaddr + phdr.p_memsz;
				}
			}
		}
	}

	current->b_heap
	 = (void*)(((ui32_t)current->b_heap + 3) & ~3);
	current->e_heap = current->b_heap;

	/** Set the registers of the current process. **/

	current->regs.gs
	= current->regs.fs
	= current->regs.es
	= current->regs.ds = 0x2b;
	current->regs.edi = current->regs.esi = 0;
	current->regs.ebp = 0;
	current->regs.ebx
	= current->regs.edx
	= current->regs.ecx
	= current->regs.eax = 0;
	current->regs.eip = ehdr.e_entry;
	current->regs.cs = 0x23;
	current->regs.eflags = 0x202;
	current->regs.esp = esp;
	current->regs.ss = 0x33;

	/** Close the files that must be closed on exec. **/

	for(fildes = 0; fildes < NR_FILDES_PER_PROC; fildes++)
	{
		fd = current->pfildes_tab[fildes];

		if(fd && (current->fildes_flags[fildes] & O_CLOEXEC))
		{
			#ifdef DEBUG
			printk("close %x -> %x on exec\n", fildes, fd->inum);
			#endif
			fildes_unref(fildes);
		}
	}

	if(file_tab[0].fs != FS_DEVFS)
	{
		panic("file table corrupted");
	}

	schedule_switch(current_pid);

	return -1; // avoid warnings from GCC
}
