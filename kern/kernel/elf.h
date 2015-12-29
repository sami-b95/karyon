#ifndef _ELF_H_
#define _ELF_H_

#include <kernel/types.h>

/** ELF header. **/

struct elf32_ehdr
{
	uchar_t e_ident[16];
	ui16_t e_type;
	ui16_t e_machine;
	ui32_t e_version;
	ui32_t e_entry;
	ui32_t e_phoff;
	ui32_t e_shoff;
	ui32_t e_flags;
	ui16_t e_ehsize;
	ui16_t e_phentsize;
	ui16_t e_phnum;
	ui16_t e_shentsize;
	ui16_t e_shnum;
	ui16_t e_shstrndx;
} __attribute__ ((packed));

/** Program header. **/

struct elf32_phdr
{
	ui32_t p_type;
	ui32_t p_offset;
	ui32_t p_vaddr;
	ui32_t p_paddr;
	ui32_t p_filesz;
	ui32_t p_memsz;
	ui32_t p_flags;
	ui32_t p_align;
} __attribute__ ((packed));

#endif
