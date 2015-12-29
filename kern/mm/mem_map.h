#ifndef _MEM_MAP_H_
#define _MEM_MAP_H_

#define GDT_BASE		0x00000000
#define IDT_BASE		0x00000800
#define KERNEL_PD_BASE		0x00001000
#define KERNEL_BASE		0x00100000
#define KERNEL_PT_BASE		0x00200000
#define PAGE_DESC_BASE		0x00300000
#define CACHE_MEMORY_BASE	0x00400000
#define PAGE_HEAP_BASE		0x20000000
#define USER_BASE		0x40000000
#define USER_STACK_BASE		0xf0000000
#define RESERVED_BASE		0xffc00000

#define in_user_range(start, limit)	((void*)(start) <= (void*)(limit) \
                                 	&& (void*)(start) >= (void*)USER_BASE \
                        	 	&& (void*)(limit) \
                                   	   <= (void*)USER_STACK_BASE)

#endif
