#ifndef _CACHE_H_
#define _CACHE_H_

#include <config.h>
#include <kernel/types.h>
#include <mm/mem_map.h>

/** Cached block structure **/

struct block
{
	bool_t used;
	ui32_t n;
	bool_t dirty;
	struct block *prev_dirty, *next_dirty;
	ui8_t buf[512];
};

/** First and last cache pages **/

#define CACHE_FIRST_VPAGE	((ui32_t)CACHE_MEMORY_BASE >> 12)
#define CACHE_LAST_VPAGE \
	(CACHE_FIRST_VPAGE + ((NR_BLOCKS * sizeof(struct block) - 1) >> 12))

/** Functions **/

void cache_init();
/****************************************************************/
ret_t cache_read(ui32_t n, void *buf);
/****************************************************************/
ret_t cache_write(ui32_t n, void *buf);
/****************************************************************/
ret_t cache_sync_block(struct block *block);
/****************************************************************/
ret_t cache_sync();

#endif
