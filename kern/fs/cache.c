/****************************************************************
 * cache.c                                                      *
 *                                                              *
 *    Block cache implementation.                               *
 *                                                              *
 ****************************************************************/

#include <config.h> // debugging
#include <fs/ata.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h> // debug
#include <mm/paging.h>

#include "cache.h"

struct block *block_tab = (void*)CACHE_MEMORY_BASE;
struct block *first_dirty = 0;
count_t dirty = 0;

/**
 * cache_init
 */

void cache_init()
{
	ui32_t ppage, vpage;
	ui32_t i;

	/** Allocate memory for the cached blocks. **/

	for(vpage = CACHE_FIRST_VPAGE; vpage <= CACHE_LAST_VPAGE; vpage++)
	{
		ppage = paging_palloc();

		if(!ppage)
		{
			panic("no physical memory left");
		}

		if(paging_map(ppage, vpage) != OK)
		{
			panic("failed mapping page");
		}
	}

	/** Set all the cached blocks as unused. **/

	for(i = 0; i < NR_BLOCKS; i++)
	{
		block_tab[i].used = 0;
	}
}

/**
 * cache_read
 */

ret_t cache_read(ui32_t n, void *buf)
{
	ui32_t hash = n % NR_BLOCKS;
	struct block *block = &block_tab[hash];

	/** Add the block to the cache if it is not already in the cache. **/

	#ifdef DEBUG
	printk("cache_read\n");
	#endif

	if(!block->used || block->n != n)
	{
		/** If the block was used, synchronize it. **/

		if(block->used)
		{
			#ifdef DEBUG
			printk("cache_read: evicting block %x, putting %x\n",
			       block->n, n);
			#endif

			if(cache_sync_block(block) != OK)
			{
				return -EIO;
			}
		}

		/** In case the read fails, setting field used to 0 invalidates
		    the block. **/

		block->used = 0;

		if(ata_read_write(ATA_CTL, ATA_SLAVE, block->buf, n, 0) != OK)
		{
			return -EIO;
		}

		block->used = 1;
		block->n = n;
		block->dirty = 0;
		block->prev_dirty = block->next_dirty = 0;

		/*#ifdef DEBUG
		printk("put block %x in cache\n", n);
		#endif*/
	}

	/** Read from the cached block. **/

	memcpy(buf, block->buf, 512);

	return OK;
}

/**
 * cache_write
 */

ret_t cache_write(ui32_t n, void *buf)
{
	ui32_t hash = n % NR_BLOCKS;
	struct block *block = &block_tab[hash];
	/** Mustn't be initialized here (first_dirty may change when
	    syncing). **/
	struct block *last_dirty;


	/** Add the block to the cache if it is not already in the cache. **/

	/*#ifdef DEBUG
	printk("cache_write\n");
	#endif*/

	if(!block->used || block->n != n)
	{
		/** If the block was used, synchronize it. **/

		if(block->used)
		{
			#ifdef DEBUG
			printk("cache_write: evict block %x put %x\n",
			       block->n, n);
			#endif

			if(cache_sync_block(block) != OK)
			{
				return -EIO;
			}
		}

		block->used = 1;
		block->n = n;

		/** Do not mark the block as dirty now (see below). **/

		block->dirty = 0;
	}

	/** Write to the cached block. **/

	memcpy(block->buf, buf, 512);

	/** If the block was not dirty, set it as dirty, add it to the dirty
	    block list and update dirty block count. **/

	if(!block->dirty)
	{
		block->dirty = 1;
		last_dirty = first_dirty ? first_dirty->prev_dirty : 0;

		if(last_dirty)
		{
			block->prev_dirty = last_dirty;
			block->next_dirty = first_dirty;
			last_dirty->next_dirty = block;
			first_dirty->prev_dirty = block;
		}
		else
		{
			first_dirty = block;
			block->prev_dirty = block->next_dirty = block;
		}

		dirty++;
	}

	/** Synchronize the first dirty block if there are too many dirty
	    blocks. **/

	if(dirty > MAX_DIRTY_BLOCKS)
	{
		if(!first_dirty)
		{
			panic("invalid dirty block count");
		}

		#ifdef CACHE_SYNC_WHEN_FULL
		if(cache_sync() != OK)
		{
			return -EIO;
		}
		#else
		if(cache_sync_block(first_dirty) != OK)
		{
			return -EIO;
		}
		#endif
	}

	return OK;
}

/**
 * cache_sync_block
 */

ret_t cache_sync_block(struct block *block)
{
	/** If the block is not used, return an error. **/

	if(!block->used)
	{
		return -EINVAL;
	}

	/** Proceed only if the block is really dirty. **/

	if(block->dirty)
	{
		/*#ifdef DEBUG
		printk("synchronizing block %x\n", block->n);
		#endif*/

		/** Write back the block. **/

		if(ata_read_write(ATA_CTL,
		                  ATA_SLAVE,
		                  block->buf,
		                  block->n,
		                  1) != OK)
		{
			return -EIO;
		}

		/** Update the dirty block list. **/

		if(block == first_dirty)
		{
			if(block->next_dirty == block)
			{
				first_dirty = 0;
			}
			else
			{
				first_dirty = block->next_dirty;
			}
		}

		if(block->next_dirty != block)
		{
			block->next_dirty->prev_dirty = block->prev_dirty;
			block->prev_dirty->next_dirty = block->next_dirty;
		}

		/** Mark the block as clean. **/

		block->dirty = 0;

		/** Update the dirty block count. **/

		dirty--;
	}

	return OK;
}

/**
 * cache_sync
 */

ret_t cache_sync()
{
	struct block *block;
	ui32_t i;

	/** Synchronize each block. **/

	for(i = 0; i < NR_BLOCKS; i++)
	{
		block = &block_tab[i];

		if(block->used && block->dirty)
		{
			if(cache_sync_block(block) != OK)
			{
				return -EIO;
			}
		}
	}

	return OK;
}
