#ifndef _PAGING_H_
#define _PAGING_H_

#include <config.h>
#include <kernel/types.h>
#include <mm/mem_map.h>

/** Page heap page **/

struct pheap_page
{
	ui32_t ppage;
	ui32_t vpage;
};

/** Constants **/

// WARNING: If NR_PPAGES is not a multiple of 8, not all pages will be
// available
#define NR_PPAGES	(RAM_SIZE / 4096)
// WARNING: immediate value
#define NR_PHEAP_PAGES	(0x20000000 / 4096)

/** Flags for page directory or page table entries **/

#define PAGING_PRESENT		0x001
#define PAGING_RW		0x002
#define PAGING_USER		0x004

/** Convenient macros **/

#define page_directory()	((ui32_t*)0xfffff000)
#define page_table(pt)		((ui32_t*)(0xffc00000 | (pt << 12)))
#define page_table_id(page)	(page >> 10)
#define page_id(page)		(page & 0x3ff)

/** Global variables (number of physical pages left, physical page reference
    counter) **/

#ifdef _PAGING_C_
size_t ppage_left = NR_PPAGES;
ui8_t *ppage_ref_cnt = (void*)PAGE_DESC_BASE;
#else
extern size_t ppage_left;
extern ui8_t *ppage_ref_cnt;
#endif

/** Functions **/

void paging_init();
/****************************************************************/
ui32_t paging_bitmap_alloc(ui8_t *bitmap, size_t bitmap_size);
/****************************************************************/
void paging_bitmap_set_used(ui8_t *bitmap, ui32_t page);
/****************************************************************/
void paging_bitmap_free(ui8_t *bitmap, ui32_t page);
/****************************************************************/
ui32_t paging_palloc();
/****************************************************************/
void paging_pfree(ui32_t ppage);
/****************************************************************/
struct pheap_page paging_valloc(bool_t map);
/****************************************************************/
void paging_vfree(struct pheap_page page);
/****************************************************************/
ret_t paging_map(ui32_t ppage, ui32_t vpage);
/****************************************************************/
void paging_unmap(ui32_t vpage);
/****************************************************************/
ui32_t *paging_create_pd();
/****************************************************************/
void paging_destroy_pd(ui32_t *pd);
/****************************************************************/
ui32_t *paging_cow_init();

#endif
