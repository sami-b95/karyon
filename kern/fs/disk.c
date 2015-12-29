/****************************************************************
 * disk.c                                                       *
 *                                                              *
 *    High level disk access.                                   *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <fs/ata.h>
#ifdef USE_CACHE
#include <fs/cache.h>
#endif
#include <kernel/errno.h>
#include <kernel/libc.h>

#include "disk.h"

/**
 * disk_read_write
 */

ret_t disk_read_write(void *buf, size_t size, off_t off, bool_t write)
{
	static ui8_t sec_buf[512];
	size_t processed = 0;
	size_t to_process;
	ui32_t cur = off / 512 + SEC_OFF;
	ret_t ret;

	while(processed < size)
	{
		to_process = min(size - processed, 512 - off % 512);
		
		if(to_process < 512 || !write)
		{	
			#ifdef USE_CACHE
			if((ret = cache_read(cur, sec_buf)) != OK)
			{
				return ret;
			}
			#else
			if((ret = ata_read_write(ATA_CTL,
			                         ATA_SLAVE,
			                         sec_buf,
			                         cur,
			                         0)) != OK)
			{
				return ret;
			}
			#endif
		}

		if(write)
		{
			memcpy(sec_buf + off % 512,
			       buf + processed,
			       to_process);

			#ifdef USE_CACHE
			if((ret = cache_write(cur, sec_buf)) != OK)
			{
				return ret;
			}
			#else
			if((ret = ata_read_write(ATA_CTL,
			                         ATA_SLAVE,
			                         sec_buf,
			                         cur,
				                 1)) != OK)
			{
				return ret;
			}
			#endif
		}
		else
		{
			memcpy(buf + processed,
			       sec_buf + off % 512,
			       to_process);
		}

		processed += to_process;
		off += to_process;
		cur++;
	}

	return OK;
}

/**
 * disk_read
 */

ret_t disk_read(void *buf, size_t size, off_t off)
{
	return disk_read_write(buf, size, off, 0);
}

/**
 * disk_write
 */

ret_t disk_write(void *buf, size_t size, off_t off)
{
	return disk_read_write(buf, size, off, 1);
}
