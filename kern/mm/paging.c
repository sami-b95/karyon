/****************************************************************
 * paging.c                                                     *
 *                                                              *
 *    Paging implementation.                                    *
 *                                                              *
 ****************************************************************/

#define _PAGING_C_
#include <config.h>
#include <kernel/errno.h>
#include <kernel/panic.h>
#include <kernel/printk.h>

#include "paging.h"

ui32_t *kpd = (void*)KERNEL_PD_BASE;
ui8_t phys_bmp[NR_PPAGES / 8] = { 0 };
ui8_t pheap_bmp[NR_PHEAP_PAGES / 8] = { 0 };

/**
 * paging_init
 */

void paging_init()
{
	ui32_t page, page_tab;
	ui32_t *pt, *pt0;
	count_t page_count;

	/** Initialize kernel page directory. **/

	for(page_tab = 0; page_tab < 1024; page_tab++)
	{
		kpd[page_tab] = 0;
	}

	/** Reserve space for kernel code and data and kernel page tables. **/

	for(page = 0; page < (PAGE_DESC_BASE >> 12); page++)
	{
		paging_bitmap_set_used(phys_bmp, page);
	}

	/** Initialize kernel page tables and register them in kernel page
	    directory. **/

	for(page_tab = 0; page_tab < (USER_BASE >> 22); page_tab++)
	{
		pt = (void*)KERNEL_PT_BASE + (page_tab << 12);

		for(page = 0; page < 1024; page++)
		{
			pt[page] = 0;
		}

		kpd[page_tab] = (ui32_t)pt | PAGING_PRESENT | PAGING_RW;
	}

	/** Identity-map kernel code and data space. **/

	pt0 = (void*)KERNEL_PT_BASE;

	for(page = 0; page < (KERNEL_PT_BASE >> 12); page++)
	{
		pt0[page] = page << 12 | PAGING_PRESENT | PAGING_RW;
	}

	/** Initialize the array giving the reference count of each physical
	    page (i.e how many times it is mapped in virtual memory).
	    page_count is the number of pages required to store the array. **/

		/** First reserve the number of physical pages required to
		    store the array and identity-map these pages. **/

	page_count = (NR_PPAGES + 4095) >> 12;

	for(page = (PAGE_DESC_BASE >> 12);
	    page < (PAGE_DESC_BASE >> 12) + page_count;
	    page++)
	{
		paging_bitmap_set_used(phys_bmp, page);
		pt0[page] = page << 12 | PAGING_PRESENT | PAGING_RW;
	}

		/** Then, initialize the array. **/

	for(page = 0; page < NR_PPAGES; page++)
	{
		ppage_ref_cnt[page] = 0;
	}

	for(page = 0;
	    page < ((PAGE_DESC_BASE >> 12) + page_count);
	    page++)
	{
		ppage_ref_cnt[page] = 1;
	}

	/** Update ppage_left. **/

	ppage_left -= ((PAGE_DESC_BASE >> 12) + page_count);

	/** Setup mirroring. **/

	kpd[1023] = KERNEL_PD_BASE | PAGING_PRESENT | PAGING_RW;

	/** Make video memory available to user. **/

	kpd[0] |= PAGING_USER;

	for(page = 0xa0; page < 0xb9; page++)
	{
		pt0[page] |= PAGING_USER;
	}

	/** Reserve the first page of the page heap. See paging_valloc for
	    further information. If the first page of the page heap is free,
	    then paging_bitmap_alloc(pheap_bmp, ...) will return 0 and this
	    will be considered an error. FIXME: Quite dirty... **/

	paging_bitmap_set_used(pheap_bmp, 0);

	/** Load page directory and enable paging. The WP bit is important: it
	    means that even the kernel cannot write to read-only pages. **/

	asm volatile("mov %0, %%eax \n\
	              mov %%eax, %%cr3 \n\
	              mov %%cr0, %%eax \n\
	              or $0x80000000, %%eax \n\
	              or $0x00010000, %%eax\n\
	              mov %%eax, %%cr0" :: "i"(KERNEL_PD_BASE));
}

/**
 * paging_bitmap_alloc
 */

ui32_t paging_bitmap_alloc(ui8_t *bitmap, size_t bitmap_size)
{
	size_t i, j;
	ui32_t page;

	for(i = 0; i < bitmap_size; i++)
	{
		if(bitmap[i] != 0xff)
		{
			for(j = 0; j < 8; j++)
			{
				if(!(bitmap[i] & (1 << j)))
				{
					page = i * 8 + j;
					paging_bitmap_set_used(bitmap, page);
					return page;
				}
			}
		}
	}

	return 0;
}

/**
 * paging_bitmap_set_used
 */

void paging_bitmap_set_used(ui8_t *bitmap, ui32_t page)
{
	bitmap[page / 8] |= (1 << (page % 8));
}

/**
 * paging_bitmap_free
 */

void paging_bitmap_free(ui8_t *bitmap, ui32_t page)
{
	bitmap[page / 8] &= ~(1 << (page % 8));
}

/**
 * paging_palloc
 */

ui32_t paging_palloc()
{
	ui32_t ppage;

	ppage = paging_bitmap_alloc(phys_bmp, NR_PPAGES / 8);

	if(ppage)
	{
		ppage_left--;
	}

	return ppage;
}

/**
 * paging_pfree
 */

void paging_pfree(ui32_t ppage)
{
	paging_bitmap_free(phys_bmp, ppage);
	ppage_left++;
}

/**
 * paging_valloc
 */

struct pheap_page paging_valloc(bool_t map)
{
	ui32_t vpage_offset;
	ui32_t ppage, vpage;
	struct pheap_page page;

	vpage_offset = paging_bitmap_alloc(pheap_bmp, NR_PHEAP_PAGES / 8);

	if(!vpage_offset)
	{
		goto fail3;
	}

	vpage = (PAGE_HEAP_BASE >> 12) + vpage_offset;
	page.vpage = vpage;

	/** Map a physical page to the virtual page if required. **/

	if(map)
	{
		ppage = paging_palloc();

		if(!ppage)
		{
			goto fail2;
		}

		if(paging_map(ppage, vpage) != OK)
		{
			goto fail1;
		}

		page.ppage = ppage;
	}
	else
	{
		page.ppage = 0;
	}

	return page;

	fail1:
		paging_pfree(ppage);
	fail2:
		paging_bitmap_free(pheap_bmp, vpage_offset);
	fail3:
		return (struct pheap_page){ 0, 0 };
}

/**
 * paging_vfree
 */

void paging_vfree(struct pheap_page page)
{
	ui32_t vpage_offset = page.vpage - (PAGE_HEAP_BASE >> 12);
	paging_bitmap_free(pheap_bmp, vpage_offset);

	if(page.ppage)
	{
		paging_unmap(page.vpage);
		paging_pfree(page.ppage);
	}
}

/**
 * paging_map
 */

ret_t paging_map(ui32_t ppage, ui32_t vpage)
{
	ui32_t pg_tab_id = page_table_id(vpage),
	       pg_id = page_id(vpage);
	ui32_t pt_page;
	ui32_t page;
	ui32_t *page_dir, *page_tab;

	page_dir = page_directory();
	page_tab = page_table(pg_tab_id);

	/** Check whether the physical page is mapped too many times. **/

	if(ppage_ref_cnt[ppage] == 255)
	{
		return -EOVERFLOW;
		//panic("physical page %x mapped too many times", ppage);
	}

	/** Do we need to allocate a page tab ? If so, try to do it. **/

	/*#ifdef DEBUG
	printk("(pg_tab_id ; pg_id) = (%x ; %x)\n", pg_tab_id, pg_id);
	#endif*/

	if(!(page_dir[pg_tab_id] & PAGING_PRESENT))
	{
		if(vpage < (USER_BASE >> 12))
		{
			panic("kernel pt are supposed to be preallocated");
		}

		/*#ifdef DEBUG
		printk("creating page table to map virtual page %x\n", vpage);
		#endif*/
		pt_page = paging_palloc();

		if(!pt_page)
		{
			return -ENOMEM;
		}

		page_dir[pg_tab_id] = pt_page << 12 | PAGING_PRESENT | PAGING_RW;

		for(page = 0; page < 1024; page++)
		{
			page_tab[page] = 0;
		}
	}

	/** Register the page in page table. **/

	if(page_tab[pg_id] & PAGING_PRESENT)
	{
		panic("page present (vpage %x, ppage %x)", vpage, ppage);
	}

	page_tab[pg_id] = ppage << 12 | PAGING_PRESENT | PAGING_RW;

	/** If the virtual page belongs to user space, add user flag to page
	    table and page descriptors. **/

	if(vpage >= (USER_BASE >> 12))
	{
		page_dir[pg_tab_id] |= PAGING_USER;
		page_tab[pg_id] |= PAGING_USER;
	}

	/** Do not forget to increase the reference counter of the physical
	    page. **/

	ppage_ref_cnt[ppage]++;

	return OK;
}

/**
 * paging_unmap
 */

void paging_unmap(ui32_t vpage)
{
	ui32_t ppage;
	ui32_t pg_tab_id = page_table_id(vpage),
	       pg_id = page_id(vpage);
	ui32_t *page_dir, *page_tab;
	#ifndef QEMU
	void *vpage_base;
	#endif

	page_dir = page_directory();

	/** DEBUG: Check whether it is legal to unmap the page. **/

	if(vpage < (PAGE_HEAP_BASE >> 12))
	{
		panic("page %x cannot be unmapped", vpage);
	}

	/** Check whether the page is mapped. **/

	if(!(page_dir[pg_tab_id] & PAGING_PRESENT))
	{
		panic("page \"belongs\" to a non-present page table");
	}

	page_tab = page_table(pg_tab_id);

	if(!(page_tab[pg_id] & PAGING_PRESENT))
	{
		panic("page is not present");
	}

	/** Unmap the page. **/

	ppage = page_tab[pg_id] >> 12;
	page_tab[pg_id] = 0;

	/** Invalidate the virtual page. **/

	#ifdef QEMU
	asm volatile("mov %cr3, %eax \n\
	              mov %eax, %cr3");
	#else
	vpage_base = (void*)(vpage << 12);

	asm volatile("invlpg %0" :: "m"(vpage_base));
	#endif

	/** Decrease the reference counter of the physical page **/

	if(ppage_ref_cnt[ppage] == 0)
	{
		panic("null reference counter for mapped page");
	}

	ppage_ref_cnt[ppage]--;

	/** If the unmapped page was mapped in user space and is not referred
	    to anywhere, free it. If the page is in kernel space, we don't
	    (see paging_create_pd to understand why: we would destroy the pd of
	    a user process as soon as created!). **/

	if(!ppage_ref_cnt[ppage] && vpage >= (USER_BASE >> 12))
	{
		#ifdef DEBUG
		printk("paging_unmap frees physical page %x\n", ppage);
		#endif
		paging_pfree(ppage);
	}
}

/**
 * paging_create_pd
 */

ui32_t *paging_create_pd()
{
	ui32_t pd_ppage; struct pheap_page pd_vpage;
	ui32_t *pd_paddr, *pd_vaddr;
	ui32_t page_tab;

	/** Allocate a physical page .**/

	pd_ppage = paging_palloc();

	if(!pd_ppage)
	{
		goto fail3;
	}

	/** Allocate a virtual page. **/

	pd_vpage = paging_valloc(0);

	if(!pd_vpage.vpage)
	{
		goto fail2;
	}

	/** Map the page directory to virtual memory. **/

	if(paging_map(pd_ppage, pd_vpage.vpage) != OK)
	{
		goto fail1;
	}

	pd_paddr = (ui32_t*)(pd_ppage << 12);
	pd_vaddr = (ui32_t*)(pd_vpage.vpage << 12);

	/** Initialize page directory. **/

	for(page_tab = 0; page_tab < 1024; page_tab++)
	{
		pd_vaddr[page_tab] = 0;
	}

	/** Map kernel space. **/

		// WARNING: immediate value.

	for(page_tab = 0; page_tab < 256; page_tab++)
	{
		pd_vaddr[page_tab] = kpd[page_tab];
	}

	/** Mirror mapping. **/

	pd_vaddr[1023] = (ui32_t)pd_paddr | PAGING_PRESENT | PAGING_RW;

	/** Unmap the page directory and free the virtual page. **/

	paging_unmap(pd_vpage.vpage);
	paging_vfree(pd_vpage);

	return pd_paddr;

	fail1:
		paging_vfree(pd_vpage);
	fail2:
		paging_pfree(pd_ppage);
	fail3:
		return 0;
}

/**
 * paging_destroy_pd
 */

void paging_destroy_pd(ui32_t *pd) // WARNING: pd must be a physical address
{
	ui32_t pg_tab_id, pg_id;
	ui32_t ppage;
	ui32_t *page_dir, *page_tab;
	ui32_t cr3 = (ui32_t)pd, old_cr3;

	asm volatile("mov %%cr3, %%eax \n\
	              mov %%eax, %0 \n\
	              mov %1, %%eax \n\
	              mov %%eax, %%cr3" : "=m"(old_cr3)
	                                : "m"(cr3)
	                                : "eax", "memory");

	page_dir = page_directory();

	/** Free pages and page tables. **/

	for(pg_tab_id = 256; pg_tab_id < 1023; pg_tab_id++)
	{
		if(page_dir[pg_tab_id] & PAGING_PRESENT)
		{
			page_tab = page_table(pg_tab_id);

			for(pg_id = 0; pg_id < 1024; pg_id++)
			{
				if(page_tab[pg_id] & PAGING_PRESENT)
				{
					ppage = page_tab[pg_id] >> 12;

					#ifdef DEBUG
					printk("page %x in pt %x\n",
					       pg_id, pg_tab_id);
					#endif

					if(!ppage_ref_cnt[ppage])
					{
						panic("null reference count");
					}

					ppage_ref_cnt[ppage]--;

					if(!ppage_ref_cnt[ppage])
					{
						paging_pfree(ppage);
					}
				}
			}

			ppage = page_dir[pg_tab_id] >> 12;

			if(ppage_ref_cnt[ppage] == 1)
			{
				panic("page table seems to be mapped");
			}
			else if(ppage_ref_cnt[ppage] > 1)
			{
				panic("pt seems to be mapped and shared");
			}

			paging_pfree(ppage);
		}
	}

	asm volatile("mov %0, %%eax \n\
	              mov %%eax, %%cr3" :
	                                : "m"(old_cr3)
	                                : "eax", "memory");

	/** Free the page directory. **/

	ppage = (ui32_t)pd >> 12;
	paging_pfree(ppage);
}

/**
 * paging_cow_init
 */

ui32_t *paging_cow_init()
{
	struct pheap_page son_pd_vpage, son_pt_vpage;
	ui32_t son_pt_ppage, ppage;
	ui32_t *page_dir, *page_tab, *son_page_dir, *son_page_tab;
	ui32_t *son_page_dir_paddr;
	ui32_t pg_tab_id, pg_id;

	/** Allocate a page directory for the son. **/

	son_page_dir_paddr = paging_create_pd();

	if(!son_page_dir_paddr)
	{
		goto fail5;
	}

	/** Allocate a virtual page to map the page directory of the son. **/

	son_pd_vpage = paging_valloc(0);

	if(!son_pd_vpage.vpage)
	{
		goto fail4;
	}

	/** Map the page directory of the son. **/

	if(paging_map((ui32_t)son_page_dir_paddr >> 12,
	              son_pd_vpage.vpage) != OK)
	{
		goto fail3;
	}

	/** Allocate a virtual page to map the page tables of the son. **/

	son_pt_vpage = paging_valloc(0);

	if(!son_pt_vpage.vpage)
	{
		goto fail2;
	}

	/** Copy page tables one by one. **/

		// WARNING: immediate value.

	page_dir = page_directory();
	son_page_dir = (ui32_t*)(son_pd_vpage.vpage << 12);
	son_page_tab = (ui32_t*)(son_pt_vpage.vpage << 12);

	for(pg_tab_id = 256; pg_tab_id < 1023; pg_tab_id++)
	{
		if(page_dir[pg_tab_id] & PAGING_PRESENT)
		{
			son_pt_ppage = paging_palloc();

			if(!son_pt_ppage)
			{
				goto fail1;
			}

			son_page_dir[pg_tab_id] = son_pt_ppage << 12
			                         | PAGING_PRESENT
			                         | PAGING_RW
			                         | PAGING_USER;

			if(paging_map(son_pt_ppage, son_pt_vpage.vpage) != OK)
			{
				/** Do not call paging_destroy_pd directly.
				    Indeed, at this point, physical page
				    son_pt_page contains unpredictable data
				    and PTs must be clean when destroying. **/

				paging_pfree(son_pt_ppage);
				son_page_dir[pg_tab_id] = 0;
				goto fail1;
			}

			page_tab = page_table(pg_tab_id);
			son_page_tab = (ui32_t*)(son_pt_vpage.vpage << 12);

			/** Stage 1: make the son's page table clean so that it
			    can be handled by paging_destroy_pd in case the
			    procedure stops midway. **/

			for(pg_id = 0; pg_id < 1024; pg_id++)
			{
				son_page_tab[pg_id] = 0;
			}

			/** Stage 2: Try to copy the parent's page table to the
			    son's.  Do not remove PAGING_RW in the parent for
			    the moment, just in case the procedure fails
			    midway. **/

			for(pg_id = 0; pg_id < 1024; pg_id++)
			{
				if(page_tab[pg_id] & PAGING_PRESENT)
				{
					//page_tab[pg_id] &= ~PAGING_RW;
					son_page_tab[pg_id]
					 = page_tab[pg_id] & ~PAGING_RW;
					ppage = page_tab[pg_id] >> 12;
					#ifdef DEBUG
					printk("pg %x in pt %x, ppage %x\n",
					       pg_id, pg_tab_id, ppage);
					#endif

					if(ppage_ref_cnt[ppage] == 255)
					{
						#ifdef DEBUG
						printk("ppage %x\n", ppage);
						#endif
						/*
						panic("cannot inc ref count");
						*/
						son_page_tab[pg_id] = 0;
						paging_unmap
						      (son_pt_vpage.vpage);
						goto fail1;
					}

					ppage_ref_cnt[ppage]++;
				}
				else
				{
					son_page_tab[pg_id] = 0;
				}
			}

			paging_unmap(son_pt_vpage.vpage);
		}
		else
		{
			son_page_dir[pg_tab_id] = 0;
		}
	}

	/** Finally remove flag PAGING_RW in the parent's page tables. **/

	for(pg_tab_id = 256; pg_tab_id < 1023; pg_tab_id++)
	{
		if(page_dir[pg_tab_id] & PAGING_PRESENT)
		{
			page_tab = page_table(pg_tab_id);

			for(pg_id = 0; pg_id < 1024; pg_id++)
			{
				if(page_tab[pg_id] & PAGING_PRESENT)
				{
					page_tab[pg_id] &= ~PAGING_RW;
				}
			}
		}
	}

	paging_vfree(son_pt_vpage);
	paging_unmap(son_pd_vpage.vpage);
	paging_vfree(son_pd_vpage);

	/** Flush TLB cache. **/

	asm volatile("mov %%cr3, %%eax \n\
	              mov %%eax, %%cr3" ::: "eax");

	return son_page_dir_paddr;

	fail1:
		paging_vfree(son_pt_vpage);
	fail2:
		paging_unmap(son_pd_vpage.vpage);
	fail3:
		paging_vfree(son_pd_vpage);
	fail4:
		paging_destroy_pd(son_page_dir_paddr);
	fail5:
		return 0;
}
