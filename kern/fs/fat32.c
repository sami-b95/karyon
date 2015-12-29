/****************************************************************
 * fat32.c                                                      *
 *                                                              *
 *    FAT32 filesystem implementation.                          *
 *                                                              *
 ****************************************************************/

#define _FAT32_C_
#include <config.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <fs/disk.h>

#include "fat32.h"

struct fat32_sb sb;
struct fat32_fsinfo fsinfo;
ui32_t cluster_size; // in bytes
ui32_t cluster_count;
ui32_t first_data_sec;
ui32_t fat_sec_buf[128];
ui32_t fat_sec;
#ifdef USE_FAT32_LIMITED_WRITE
bool_t fat_sec_dirty;
#endif
#ifdef USE_FAT32_FAST_LOOKUP

/** This variable is supposed to be initialized correctly the first time
    fat32_alloc_clusters or fat32_free_clusters is called. **/

ui32_t first_free_cluster = 2;
#endif
struct fat32_dent empty_dent = {
	.file_name = "\0          ",
	.attr = 0,
	.creat_time_tenth_sec = 0,
	.creat_time = 0,
	.creat_date = 0,
	.last_acc_date = 0,
	.first_cluster_high = 0,
	.last_mod_time = 0,
	.last_mod_date = 0,
	.first_cluster_low = 0,
	.file_size = 0
};

/**
 * fat32_init
 */

void fat32_init()
{
	/** Read the superblock. **/

	if(disk_read(&sb, 512, 0) != OK)
	{
		panic("failed reading FAT32 superblock");
	}

	/** Read FS information sector. **/

	if(disk_read(&fsinfo, 512, sb.fsinfo_sec * 512) != OK)
	{
		panic("failed reading FSinfo sector");
	}

	/** Read the first sector of the first FAT. **/

	if(disk_read(fat_sec_buf, 512, sb.res_sec_cnt * 512) != OK)
	{
		panic("failed reading the first sector of the FAT");
	}

	fat_sec = 0;
	#ifdef USE_FAT32_LIMITED_WRITE
	fat_sec_dirty = 0;
	#endif

	/** Extract some useful information. **/

	cluster_size = sb.sec_per_cluster * 512;
	first_data_sec = sb.res_sec_cnt
	                 + sb.fat_cnt * sb.sec_per_fat
	                 + (sb.root_dent_cnt * 32 + 511) / 512;
	cluster_count = (sb.large_sec_cnt - first_data_sec)
	                / sb.sec_per_cluster;
	root_dir.dent_cluster = 0;
	root_dir.dent_off = 0;
	root_dir.dent.attr = FAT32_ATTR_DIRECTORY;
	root_dir.dent.creat_time_tenth_sec = 0;
	root_dir.dent.creat_time = 0;
	root_dir.dent.creat_date = 0;
	root_dir.dent.last_acc_date = 0;
	root_dir.dent.first_cluster_high = sb.root_dir_cluster >> 16;
	root_dir.dent.last_mod_time = 0;
	root_dir.dent.last_mod_date = 0;
	root_dir.dent.first_cluster_low = sb.root_dir_cluster & 0xffff;
	root_dir.dent.file_size = 0;

	if(!cluster_count)
	{
		/** This should not happen if we have a well-formatted FAT32
		    filesystem. **/

		panic("invalid FAT32 filesystem");
	}
}

/**
 * fat32_sync
 */

ret_t fat32_sync()
{
	ui32_t fat;

	/** Synchronize buffered FAT sector. **/

	#ifdef DEBUG
	printk("fat_sec = %x\n", fat_sec);
	#endif

	for(fat = 0; fat < sb.fat_cnt; fat++)
	{
		if(disk_write(fat_sec_buf,
		              512,
		              (sb.res_sec_cnt + fat * sb.sec_per_fat
		               + fat_sec) * 512) != OK)
		{
			return -EIO;
		}

		#ifdef USE_FAT32_LIMITED_WRITE
		fat_sec_dirty = 0;
		#endif
	}

	/** Synchronize FS information sector. **/

	if(disk_write(&fsinfo, 512, sb.fsinfo_sec * 512) != OK)
	{
		return -EIO;
	}

	return OK;
}

/**
 * fat32_buffer_fat
 */

ret_t fat32_buffer_fat(ui32_t new_fat_sec)
{
	ui32_t fat;

	#ifdef DEBUG
	printk("[buffer fat segment]\n");
	#endif

	#ifdef USE_FAT32_LIMITED_WRITE
	if(fat_sec_dirty)
	{
	#endif
		for(fat = 0; fat < sb.fat_cnt; fat++)
		{
			if(disk_write(fat_sec_buf,
			              512,
			              (sb.res_sec_cnt + fat * sb.sec_per_fat
			               + fat_sec) * 512) != OK)
			{
				return -EIO;
			}
		}
	#ifdef USE_FAT32_LIMITED_WRITE
	}
	#endif

	/** Read from the first FAT. **/

	if(disk_read(fat_sec_buf,
	             512,
	             (sb.res_sec_cnt + new_fat_sec) * 512) != OK)
	{
		return -EIO;
	}

	fat_sec = new_fat_sec;

	return OK;
}

/**
 * fat32_get_cluster_desc
 */

ret_t fat32_get_cluster_desc(ui32_t cluster, ui32_t *cluster_desc)
{
	ui32_t new_fat_sec = cluster / 128; // 128 descriptors per sector.
	ui32_t desc_off = cluster % 128;

	/** Check whether the FAT sector we want to access is loaded to memory.
	    If not, write back the previously loaded sector and load the new
	    one. **/

	if(new_fat_sec != fat_sec)
	{
		if(fat32_buffer_fat(new_fat_sec) != OK)
		{
			return -EIO;
		}
	}

	/** Get the descriptor. **/

	*cluster_desc = fat_sec_buf[desc_off] & 0x0fffffff;

	return OK;
}

/**
 * fat32_set_cluster_desc
 */

ret_t fat32_set_cluster_desc(ui32_t cluster, ui32_t cluster_desc)
{
	ui32_t new_fat_sec = cluster / 128; // 128 descriptors per sector.
	ui32_t desc_off = cluster % 128;

	/** Check whether the FAT sector we want to access is loaded to memory.
	    If not, write back the previously loaded sector and load the new
	    one. **/

	if(new_fat_sec != fat_sec)
	{
		if(fat32_buffer_fat(new_fat_sec) != OK)
		{
			return -EIO;
		}
	}

	/** Set the descriptor. **/

	fat_sec_buf[desc_off] = cluster_desc;
	#ifdef USE_FAT32_LIMITED_WRITE
	fat_sec_dirty = 1;
	#endif

	return OK;
}

/**
 * fat32_alloc_clusters
 */

ui32_t fat32_alloc_clusters(count_t count)
{
	ui32_t first, prev, cur;
	ui32_t desc;
	count_t alloc; // allocated clusters

	/** FIXME When testing cur < cluster_count, there may be two unusable
	    clusters. But to confirm it, it is necessary to check that the
	    calculation of cluster_count is correct. **/

	#ifdef USE_FAT32_FAST_LOOKUP
	#define FAT32_INIT_CUR	first_free_cluster
	#else
	#define FAT32_INIT_CUR	2
	#endif

	for(alloc = 0, first = 0, prev = 0, cur = FAT32_INIT_CUR;
	    cur < cluster_count && alloc < count;
	    cur++)
	{
		if(fat32_get_cluster_desc(cur, &desc) != OK)
		{
			goto fail;
		}

		if(!desc)
		{
			/** The cluster is free. Mark it as the last cluster of
			    the chain. **/

			if(fat32_set_cluster_desc(cur, 0x0fffffff) != OK)
			{
				goto fail;
			}

			if(!first)
			{
				/** The cluster is the first we could
				    allocate. **/

				first = cur;
			}
			else
			{
				/** The first cluster was already allocated.
				    There are more than one cluster, so we can
				    update the descriptor of the previous
				    cluster. **/

				if(!prev)
				{
					/** This should not happen. **/

					panic("internal error");
				}

				if(fat32_set_cluster_desc(prev, cur) != OK)
				{
					/** Free the current cluster, as it
					    could not be integrated into the
					    chain and thus cannot be freed with
					    fat32_free_clusters(first). **/

					if(fat32_free_clusters(cur) != OK)
					{
						panic("failed freeing clust.");
					}

					goto fail;
				}
			}

			prev = cur;
			alloc++;
		}
	}

	if(alloc < count)
	{
		#ifdef DEBUG
		printk("not enough clusters\n");
		#endif
		goto fail;
	}

	fsinfo.alloc_clusters += count;
	fsinfo.free_clusters -= count;

	#ifdef USE_FAT32_FAST_LOOKUP
	first_free_cluster = cur;
	#endif

	return first;

	fail:
		if(first)
		{
			if(fat32_free_clusters(first) != OK)
			{
				panic("could not free clusters\n");
			}
		}

		return 0;
}

/**
 * fat32_free_clusters
 */

ret_t fat32_free_clusters(ui32_t first)
{
	ui32_t cur;
	ui32_t desc;
	count_t freed = 0;

	if(first < 2)
	{
		panic("invalid cluster %x", first);
	}

	#ifdef DEBUG
	printk("freeing from cluster %x\n", first);
	#endif

	cur = first;

	do
	{
		#ifdef DEBUG
		printk("freeing %x\n", cur);
		#endif

		if(fat32_get_cluster_desc(cur, &desc) != OK)
		{
			return -EIO;
		}

		if(fat32_set_cluster_desc(cur, 0) != OK)
		{
			return -EIO;
		}

		cur = desc;
		freed++;

	} while(desc < 0x0ffffff8);

	#ifdef DEBUG
	printk("freed: %x\n", freed);
	#endif

	fsinfo.alloc_clusters -= freed;
	fsinfo.free_clusters += freed;

	#ifdef USE_FAT32_FAST_LOOKUP
	if(first < first_free_cluster)
	{
		first_free_cluster = first;
	}
	#endif

	return OK;
}

/**
 * fat32_get_cluster_n
 */

ui32_t fat32_get_cluster_n(ui32_t first, ui32_t n)
{
	ui32_t cur = first;
	ui32_t desc;

	while(n--)
	{
		if(fat32_get_cluster_desc(cur, &desc) != OK)
		{
			return 0;
		}

		if(desc >= 0x0ffffff8)
		{
			return desc;
		}

		cur = desc;
	}

	return cur;
}

/**
 * fat32_get_last_cluster
 */

ui32_t fat32_get_last_cluster(ui32_t first)
{
	ui32_t cur = first, prev;
	ui32_t desc;

	while(cur < 0x0ffffff8)
	{
		if(fat32_get_cluster_desc(cur, &desc) != OK)
		{
			return 0;
		}

		prev = cur;
		cur = desc;
	}

	return prev;
}

/**
 * fat32_read_cluster
 */

ret_t fat32_read_cluster(ui32_t cluster, void *buf, size_t size, off_t off)
{
	return disk_read(buf,
	                 size,
	                 first_data_sec * 512
	                 + (cluster - 2) * cluster_size
	                 + off);
}

/**
 * fat32_write_cluster
 */

ret_t fat32_write_cluster(ui32_t cluster, void *buf, size_t size, off_t off)
{
	return disk_write(buf,
	                  size,
	                  first_data_sec * 512
	                  + (cluster - 2) * cluster_size
	                  + off);
}

/**
 * fat32_read_write
 *
 * WARNING: Can read/write all the clusters reserved for the file in their
 *          entirety, even the last one. Thus, the read/written bytes may not
 *          reflect the data contained in the file. Some control at upper level
 *          is necessary.
 */

ssize_t fat32_read_write(ui32_t first_cluster,
                         void *buf,
                         size_t size,
                         off_t off,
                         bool_t write)
{
	ui32_t n = off / cluster_size;
	ui32_t first_cluster_to_process, cur;
	ui32_t desc;
	size_t bytes_to_process;
	ssize_t processed_bytes = 0;
	ret_t (*func)(ui32_t, void*, size_t, off_t)
	 = write ? fat32_write_cluster : fat32_read_cluster;

	/** If the file is empty, return now. **/

	if(!first_cluster)
	{
		return 0;
	}

	/** Get the first cluster to process. **/

	first_cluster_to_process = fat32_get_cluster_n(first_cluster, n);

	if(!first_cluster_to_process
	|| first_cluster_to_process >= 0x0ffffff8)
	{
		#ifdef DEBUG
		printk("%x, %x, %x, %x, %x\n",
		       first_cluster, buf, size, off, write);
		printk("error 1 (first cluster to process: %x)\n",
		       first_cluster_to_process);
		#endif
		return -1;
	}

	cur = first_cluster_to_process;

	/** Deal with the case in which the offset is not cluster-aligned. **/

	if(off % cluster_size)
	{
		#ifdef DEBUG
		printk("read/write first cluster\n");
		#endif

		bytes_to_process
		 = size >= (cluster_size - (off % cluster_size))
		   ? (cluster_size - (off % cluster_size))
		   : size;

		if(func(first_cluster_to_process,
		        buf,
		        size,
		        off % cluster_size) != OK)
		{
			#ifdef DEBUG
			printk("error 2");
			#endif
			return -1;
		}

		processed_bytes = bytes_to_process;

		if(fat32_get_cluster_desc(first_cluster_to_process,
		                          &desc) != OK)
		{
			#ifdef DEBUG
			printk("error 3");
			#endif
			return -1;
		}

		cur = desc;
	}

	/** Read whole clusters as long as possible. **/

	while((size - processed_bytes) >= cluster_size && cur < 0x0ffffff8)
	{
		#ifdef DEBUG
		printk("read/write whole cluster\n");
		#endif

		if(func(cur, buf + processed_bytes, cluster_size, 0) != OK)
		{			
			#ifdef DEBUG
			printk("error 4");
			#endif
			return -1;
		}

		processed_bytes += cluster_size;

		if(fat32_get_cluster_desc(cur, &desc) != OK)
		{
			#ifdef DEBUG
			printk("error 5");
			#endif
			return -1;
		}

		cur = desc;
	}

	if(cur >= 0x0ffffff8)
	{
		return processed_bytes;
	}

	/** Partially read the last cluster if required. **/

	if(processed_bytes < size)
	{
		#ifdef DEBUG
		printk("read/write last cluster\n");
		#endif

		bytes_to_process = size - processed_bytes;

		if(func(cur, buf + processed_bytes, bytes_to_process, 0) != OK)
		{
			#ifdef DEBUG
			printk("error 6");
			#endif
			return -1;
		}

		processed_bytes += bytes_to_process;
	}

	return processed_bytes;
}

/**
 * fat32_read
 */

ssize_t fat32_read(struct fat32_file *file, void *buf, size_t size, off_t off)
{
	ui32_t first_cluster = file->dent.first_cluster_high << 16
	                     | file->dent.first_cluster_low;

	return fat32_read_write(first_cluster, buf, size, off, 0);
}

/**
 * fat32_write
 */

ssize_t fat32_write(struct fat32_file *file, void *buf, size_t size, off_t off)
{
	ui32_t first_cluster = file->dent.first_cluster_high << 16
	                     | file->dent.first_cluster_low;

	return fat32_read_write(first_cluster, buf, size, off, 1);
}

/**
 * fat32_append
 */

ret_t fat32_append(struct fat32_file *file, size_t bytes)
{
	count_t to_add; // number of clusters to add
	ui32_t first_cluster = file->dent.first_cluster_high << 16
	                     | file->dent.first_cluster_low;
	ui32_t last_cluster = 0; // must be initialized (see fail1)
	ui32_t first_alloc_cluster;

	/** Calculate the number of clusters to add. **/

	to_add = (file->dent.file_size + bytes + cluster_size - 1)
	          / cluster_size
	       - (file->dent.file_size + cluster_size - 1)
	          / cluster_size;

	/** Allocate and map clusters if necessary. **/

	if(to_add)
	{
		#ifdef DEBUG
		printk("[alloc cluster(s)] ");
		#endif

		first_alloc_cluster = fat32_alloc_clusters(to_add);

		if(!first_alloc_cluster)
		{
			goto fail3;
		}

		#ifdef DEBUG
		printk("[map cluster] ");
		#endif

		if(file->dent.file_size)
		{
			/** Find the last cluster of the file. **/

			last_cluster = fat32_get_last_cluster(first_cluster);

			if(!last_cluster)
			{
				goto fail2;
			}

			/** Append the allocated clusters to the cluster
			    chain. **/

			if(fat32_set_cluster_desc(last_cluster,
				                  first_alloc_cluster) != OK)
			{
				goto fail2;
			}
		}
		else
		{
			file->dent.first_cluster_high
			 = first_alloc_cluster >> 16;
			file->dent.first_cluster_low
			 = first_alloc_cluster & 0xffff;
		}
	}

	#ifdef DEBUG
	printk("[update dent]\n");
	#endif

	/** Update the directory entry and write it to the disk. **/

	file->dent.file_size += bytes;

	if(fat32_write_cluster(file->dent_cluster,
	                       &file->dent,
	                       32,
	                       file->dent_off) != OK)
	{
		goto fail1;
	}

	return OK;

	fail1:
		if(to_add && last_cluster)
		{
			if(fat32_set_cluster_desc(last_cluster,
			                          0x0fffffff) != OK)
			{
				panic("failed restoring previous eof");
			}
		}
	fail2:
		if(to_add)
		{
			if(fat32_free_clusters(first_alloc_cluster) != OK)
			{
				panic("failed freeing clusters");
			}
		}
	fail3:
		return -EIO;
}

/**
 * fat32_truncate
 */

ret_t fat32_truncate(struct fat32_file *file, size_t size)
{
	count_t to_free;
	ui32_t n;
	ui32_t first_cluster = file->dent.first_cluster_high << 16
	                     | file->dent.first_cluster_low;
	ui32_t first_cluster_to_free, new_last_cluster;
	ui32_t desc;

	if(size > file->dent.file_size)
	{
		return -EINVAL;
	}

	/** Calculate the number of clusters to free. **/

	to_free = (file->dent.file_size + cluster_size - 1) / cluster_size
	        - (size + cluster_size - 1) / cluster_size;

	/** Free clusters if necessary. **/

	if(to_free)
	{
		if(size)
		{
			n = (size + cluster_size - 1) / cluster_size - 1;
			new_last_cluster = fat32_get_cluster_n(first_cluster,
			                                       n);

			if(!new_last_cluster || new_last_cluster >= 0x0ffffff8)
			{
				return -EIO;
			}

			if(fat32_get_cluster_desc(new_last_cluster,
			                          &desc) != OK)
			{
				return -EIO;
			}

			if(desc >= 0x0ffffff8)
			{
				return -EIO;
			}

			first_cluster_to_free = desc;

			if(fat32_free_clusters(first_cluster_to_free) != OK)
			{
				return -EIO;
			}

			if(fat32_set_cluster_desc(new_last_cluster,
			                          0x0fffffff) != OK)
			{
				return -EIO;
			}
		}
		else
		{
			if(fat32_free_clusters(first_cluster) != OK)
			{
				return -EIO;
			}

			file->dent.first_cluster_high = 0;
			file->dent.first_cluster_low = 0;
		}
	}

	/** Update the directory entry and write it to the disk. **/

	file->dent.file_size = size;

	if(!size)
	{
		file->dent.first_cluster_high
		= file->dent.first_cluster_low = 0;
	}

	if(fat32_write_cluster(file->dent_cluster,
	                       &file->dent,
	                       32,
	                       file->dent_off) != OK)
	{
		return -EIO;
	}

	return OK;
}

/**
 * fat32_convert_name
 */

ret_t fat32_convert_name(uchar_t *src, uchar_t *dest)
{
	size_t src_len = strlen(src);
	ui32_t i, j;

	/** Check whether the name is too long. **/

	if((!strchr(src, '.') && src_len > 8)
	   || (strchr(src, '.') && src_len > 12))
	{
		return -EINVAL;
	}

	/** Check whether the name contains forbidden characters. **/

	if(strchr(src, '"') || strchr(src, '*') || strchr(src, '/')
	|| strchr(src, ':') || strchr(src, '<') || strchr(src, '>')
	|| strchr(src, '?') || strchr(src, '\\') || strchr(src, '|')
	|| strchr(src, '+') || strchr(src, ',') || strchr(src, ';')
	|| strchr(src, '=') || strchr(src, '[') || strchr(src, ']')
	|| strchr(src, 0x7f))
	{
		return -EINVAL;
	}

	for(i = 0; i < src_len; i++)
	{
		/** Control characters not allowed. **/

		if(src[i] < 32)
		{
			return -EINVAL;
		}
	}

	/** Convert the name. **/

	for(i = 0, j = 0; src[i] != '\0' && j < 11; i++)
	{
		if(src[i] >= 'a' && src[i] <= 'z')
		{
			dest[j++] = src[i] - 'a' + 'A';
		}
		else if(src[i] == 0xe5)
		{
			dest[j++] = 0x05;
		}
		else if(src[i] == '.')
		{
			for(; j < 8; j++)
			{
				dest[j] = ' ';
			}
		}
		else
		{
			dest[j++] = src[i];
		}
	}

	for(; j < 11; j++)
	{
		dest[j] = ' ';
	}

	return OK;
}

/**
 * fat32_look_for_dent
 */

ret_t fat32_look_for_dent(struct fat32_file *dir,
                          struct fat32_file *file,
                          uchar_t *name)
{
	ui32_t desc;
	uchar_t fmt_name[11]; // formatted name

	file->dent_cluster = dir->dent.first_cluster_high << 16
	                   | dir->dent.first_cluster_low;

	if(fat32_convert_name(name, fmt_name) != OK)
	{
		return -EINVAL;
	}


	while(file->dent_cluster < 0x0ffffff8)
	{
		for(file->dent_off = 0;
		    file->dent_off < cluster_size;
		    file->dent_off += 32)
		{
			if(fat32_read_cluster(file->dent_cluster,
			                      &file->dent,
			                      32,
			                      file->dent_off) != OK)
			{
				return -EIO;
			}

			/*#ifdef DEBUG
			printk("\"%s\"\n", file->dent.file_name);
			#endif*/

			if(file->dent.file_name[0] == '\0')
			{
				/** We have reached the end of the
				    directory. **/

				return -ENOENT;
			}
			else if(file->dent.file_name[0] == 0xe5)
			{
				/** The entry is invalid. **/

				continue;
			}
			else if(file->dent.attr == 0x0f)
			{
				/** Long file name metadata. **/

				continue;
			}
			else if(strncmp((uchar_t*)file->dent.file_name,
			                fmt_name,
			                11) == 0)
			{
				/** We have found the entry. **/

				return OK;
			}
		}

		/** Go to next cluster if the current one is not the last. **/

		if(fat32_get_cluster_desc(file->dent_cluster, &desc) != OK)
		{
			return -EIO;
		}

		file->dent_cluster = desc;
	}

	/** Normally, we should not come here, because there should be a null
	    entry at the end of the directory. **/

	panic("no null entry at the end of directory \"%s\"", dir->dent.file_name);

	return -EIO; // to avoid warnings from GCC
}

/**
 * fat32_count_dent
 */

count_t fat32_count_dent(struct fat32_file *dir)
{
	ui32_t cur;
	ui32_t desc;
	struct fat32_dent dent;
	off_t dent_off;
	count_t count = 0;

	cur = dir->dent.first_cluster_high << 16
	    | dir->dent.first_cluster_low;

	while(cur < 0x0ffffff8)
	{
		for(dent_off = 0; dent_off < cluster_size; dent_off += 32)
		{
			if(fat32_read_cluster(cur, &dent, 32, dent_off) != OK)
			{
				return 0;
			}

			if(dent.file_name[0] == '\0')
			{
				/** We have reached the end of the
				    directory. **/

				return count;
			}
			else if(dent.file_name[0] == 0xe5 || dent.attr == 0x0f)
			{
				/** Deleted entry or long file name
				    metadata. **/

				continue;
			}
			else
			{
				/** A directory entry was found. **/
				/*#ifdef DEBUG
				printk("counting entry \"%s\"\n",
				       dent.file_name);
				printk("dent.file_name[0] = %x\n",
				       dent.file_name[0]);
				#endif*/

				count++;
			}
		}

		/** Go to next cluster if the current one is not the last. **/

		if(fat32_get_cluster_desc(cur, &desc) != OK)
		{
			return 0;
		}

		cur = desc;
	}

	/** Normally, we should not come here, because there should be a null
	    entry at the end of the directory. **/

	panic("no null entry at the end of directory \"%s\"", dir->dent.file_name);

	return -EIO; // to avoid warnings from GCC
}

/**
 * fat32_create
 */

ret_t fat32_create(struct fat32_file *dir,
                   struct fat32_file *file,
                   uchar_t *name,
                   bool_t is_dir)
{
	ui32_t desc;
	ui32_t first_dir_cluster = 0; // must be initialized (see fail1)
	ui32_t alloc_dir_cluster;
	struct fat32_dent tmp_dent;
	ret_t error;

	file->dent_cluster = dir->dent.first_cluster_high << 16
	                   | dir->dent.first_cluster_low;

	/** Check whether dir does refer to a directory. **/

	if(!(dir->dent.attr & FAT32_ATTR_DIRECTORY))
	{
		error = EINVAL;
		goto fail2;
	}

	/** Check for the validity of the name. **/

	if(fat32_convert_name(name, (uchar_t*)file->dent.file_name) != OK)
	{
		error = ENAMETOOLONG;
		goto fail2;
	}

	/** If the file is a directory, allocate one cluster and write some
	    directory entries: ".", ".." and empty entries to mark the end of
	    the directory. **/

	if(is_dir)
	{
		off_t dent_off = 64;

		first_dir_cluster = fat32_alloc_clusters(1);

		if(!first_dir_cluster)
		{
			/** FIXME Find a more correct error code. **/

			error = ENOMEM;
			goto fail2;
		}

		struct fat32_dent dent1 = {
			.file_name = ".          ",
			.attr = FAT32_ATTR_DIRECTORY,
			.creat_time_tenth_sec = 0,
			.creat_time = 0,
			.creat_date = 0,
			.last_acc_date = 0,
			.first_cluster_high = first_dir_cluster >> 16,
			.last_mod_time = 0,
			.last_mod_date = 0,
			.first_cluster_low = first_dir_cluster & 0xffff,
			.file_size = 0
		};

		struct fat32_dent dent2 = {
			.file_name = "..         ",
			.attr = dir->dent.attr,
			.creat_time_tenth_sec = dir->dent.creat_time_tenth_sec,
			.creat_time = dir->dent.creat_time,
			.creat_date = dir->dent.creat_date,
			.last_acc_date = dir->dent.last_acc_date,
			.first_cluster_high = !memcmp(dir, &root_dir, 32)
			                     ? 0
			                     : dir->dent.first_cluster_high,
			.last_mod_time = dir->dent.last_mod_time,
			.last_mod_date = dir->dent.last_mod_date,
			.first_cluster_low = !memcmp(dir, &root_dir, 32)
			                     ? 0
			                     : dir->dent.first_cluster_low,
			.file_size = 0
		};

		if(fat32_write_cluster(first_dir_cluster,
			               &dent1,
			               32,
			               0) != OK
		|| fat32_write_cluster(first_dir_cluster,
			               &dent2,
			               32,
			               32) != OK)
		{
			error = EIO;
			goto fail1;
		}

		while(dent_off < cluster_size)
		{
			if(fat32_write_cluster(first_dir_cluster,
			                       &empty_dent,
			                       32,
			                       dent_off) != OK)
			{
				error = EIO;
				goto fail1;
			}

			dent_off += 32;
		}
	}

	/** Look for a free directory entry. **/

	while(file->dent_cluster < 0x0ffffff8)
	{
		for(file->dent_off = 0;
		    file->dent_off < cluster_size;
		    file->dent_off += 32)
		{
			if(fat32_read_cluster(file->dent_cluster,
			                      &tmp_dent,
			                      32,
			                      file->dent_off) != OK)
			{
				error = EIO;
				goto fail1;
			}

			if(tmp_dent.file_name[0] == '\0'
			|| tmp_dent.file_name[0] == 0xe5)
			{
				/** Null entry at the end of the directory or
				    previously deleted entry. In both cases,
				    the entry is free. **/

				break;
			}
		}

		if(fat32_get_cluster_desc(file->dent_cluster, &desc) != OK)
		{
			error = EIO;
			goto fail1;
		}

		if(file->dent_off == cluster_size)
		{
			/** We have come to the end of the cluster. Go to the
			    next one if the current cluster is not the last of
			    the chain. **/

			file->dent_cluster = desc;
		}
		else
		{
			/** We have found a free directory entry. **/

			break;
		}
	}

	if(file->dent_cluster >= 0x0ffffff8)
	{
		panic("no null entry at the end of the directory");
	}

	if(tmp_dent.file_name[0] == '\0')
	{
		ui32_t empty_dent_cluster;
		off_t empty_dent_off;

		#ifdef DEBUG
		printk("end of directory\n");
		#endif

		if(file->dent_off + 32 == cluster_size)
		{
			/** We cannot write the empty directory entry at the
			    end of the current cluster. **/

			if(desc >= 0x0ffffff8)
			{
				/** The current cluster is the last one.
				    Allocate a new cluster. **/

				#ifdef DEBUG
				printk("allocating a new cluster\n");
				#endif

				alloc_dir_cluster = fat32_alloc_clusters(1);

				if(!alloc_dir_cluster)
				{
					/** FIXME See above. **/

					error = ENOMEM;
					goto fail1;
				}

				if(fat32_set_cluster_desc(
				                    file->dent_cluster,
				                    alloc_dir_cluster) != OK)
				{
					if(fat32_free_clusters(
					              alloc_dir_cluster) != OK)
					{
						panic("failed freeing clust.");
					}

					error = EIO;
					goto fail1;
				}

				empty_dent_cluster = alloc_dir_cluster;
			}
			else
			{
				/** The empty directory entry must go into the
				    next cluster. **/

				empty_dent_cluster = desc;
			}

			empty_dent_off = 0;
		}
		else
		{
			empty_dent_cluster = file->dent_cluster;
			empty_dent_off = file->dent_off + 32;
		}

		/** Write empty entries to mark the end of the directory. **/

		while(empty_dent_off < cluster_size)
		{
			if(fat32_write_cluster(empty_dent_cluster,
			                       &empty_dent,
			                       32,
			                       empty_dent_off) != OK)
			{
				error = EIO;
				goto fail1;
			}

			empty_dent_off += 32;
		}
	}

	/** Prepare the directory entry. **/

	// Name already initialized at the beginning
	file->dent.attr = is_dir ? FAT32_ATTR_DIRECTORY : 0;
	file->dent.reserved = 0;
	file->dent.creat_time_tenth_sec = 0;
	file->dent.creat_time = 0;
	file->dent.creat_date = 0;
	file->dent.last_acc_date = 0;
	file->dent.first_cluster_high
	 = is_dir ? (first_dir_cluster >> 16) : 0;
	file->dent.last_mod_time = 0;
	file->dent.last_mod_date = 0;
	file->dent.first_cluster_low
	 = is_dir ? (first_dir_cluster & 0xffff) : 0;
	file->dent.file_size = 0;

	/*#ifdef DEBUG
	printk("current cluster: %x\n", cur);
	printk("dent offset: %x\n", file->dent_off);
	#endif*/

	/** Write the directory entry. **/

	if(fat32_write_cluster(file->dent_cluster,
	                       &file->dent,
	                       32,
	                       file->dent_off) != OK)
	{
		error = EIO;
		goto fail1;
	}

	return OK;

	fail1:
		if(first_dir_cluster)
		{
			if(fat32_free_clusters(first_dir_cluster) != OK)
			{
				panic("failed freeing clusters\n");
			}
		}
	fail2:
		return -error;
}

/**
 * fat32_remove
 */

ret_t fat32_remove(struct fat32_file *file)
{
	ui32_t first_cluster = file->dent.first_cluster_high << 16
	                     | file->dent.first_cluster_low;

	/** If the file is a non-empty directory, do not remove it. **/

	if((file->dent.attr & FAT32_ATTR_DIRECTORY)
	   && fat32_count_dent(file) > 2)
	{
		#ifdef DEBUG
		printk("non-empty directory (%x directory entries)\n",
		       fat32_count_dent(file));
		#endif
		return -EINVAL;
	}

	/** If any cluster is used by the file, free the cluster(s) used by the
	    file. **/

	if(first_cluster)
	{
		if(fat32_free_clusters(first_cluster) != OK)
		{
			return -EIO;
		}
	}

	/** Mark the directory entry as removed. **/

	file->dent.file_name[0] = 0xe5;

	if(fat32_write_cluster(file->dent_cluster,
	                       &file->dent,
	                       32,
	                       file->dent_off) != OK)
	{
		return -EIO;
	}

	return OK;
}

/**
 * fat32_link
 */

ret_t fat32_link(struct fat32_file *src, struct fat32_file *dest)
{
	/** Check whether clusters are not already allocated for file dest. **/

	if(dest->dent.first_cluster_high || dest->dent.first_cluster_low)
	{
		return -EINVAL;
	}

	/** Update the first cluster of the destination file. **/

	dest->dent.first_cluster_high = src->dent.first_cluster_high;
	dest->dent.first_cluster_low = src->dent.first_cluster_low;
	dest->dent.file_size = src->dent.file_size;

	return fat32_write_cluster(dest->dent_cluster,
	                           &dest->dent,
	                           32,
	                           dest->dent_off);
}

/**
 * fat32_unlink
 */

ret_t fat32_unlink(struct fat32_file *file)
{
	file->dent.file_name[0] = 0xe5;

	return fat32_write_cluster(file->dent_cluster,
	                           &file->dent,
	                           32,
	                           file->dent_off);
}
