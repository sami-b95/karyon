/****************************************************************
 * ext2.c                                                       *
 *                                                              *
 *    Ext2 filesystem driver.                                   *
 *                                                              *
 ****************************************************************/

#define _EXT2_C_
#include <config.h>
#include <fs/disk.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/panic.h>

#include <fs/ext2.h>

ui32_t block_size;
ui16_t inode_size;
ui32_t bg_table;
/** Functions allocating a block/inode should respectively start with this
    block/inode. It's supposed to speed up allocation. **/
ui32_t start_block = 3, start_inode = 3;

/**
 * ext2_init
 */

void ext2_init()
{
	/** Read the superblock. **/

	if(disk_read(&sb, sizeof(struct ext2_sb), 1024) != OK)
	{
		panic("failed reading ext2 superblock");
	}

	/** Check Ext2 signature. **/

	if(sb.s_sig != 0xef53)
	{
		panic("bad ext2 signature");
	}

	/** Get inode size and block size. **/

	if(sb.s_ver_maj < 1)
	{
		inode_size = 128;
	}
	else
	{
		if(sb.s_inode_size != 128)
		{
			panic("only revision 0 file systems are supported");
		}

		inode_size = sb.s_inode_size;
	}

	block_size = 1024 << sb.s_log_block_size;

	/** Store the block group table location. **/

	if(block_size == 1024)
	{
		bg_table = 2;
	}
	else
	{
		bg_table = 1;
	}
}

/**
 * ext2_sync
 */

ret_t ext2_sync()
{
	/** Synchronize the superblock. **/

	if(disk_write(&sb, sizeof(struct ext2_sb), 1024) != OK)
	{
		return -EIO;
	}

	return OK;
}

/**
 * ext2_read_block
 */

ret_t ext2_read_block(ui32_t block, void *buf, size_t size, off_t off)
{
	return disk_read(buf, size, block * block_size + off);
}

/**
 * ext2_write_block
 */

ret_t ext2_write_block(ui32_t block, void *buf, size_t size, off_t off)
{
	if(off + size > block_size)
	{
		panic("won't write over block boundary");
	}

	return disk_write(buf, size, block * block_size + off);
}

/**
 * ext2_zero_block
 */

ret_t ext2_zero_block(ui32_t block)
{
	static ui8_t zero[1024] = { 0 };
	ui32_t off;
	ret_t ret;

	for(off = 0; off < block_size; off += 1024)
	{
		ret = ext2_write_block(block, zero, 1024, off);

		if(ret != OK)
		{
			break;
		}
	}

	return ret;
}

/**
 * ext2_read_table
 */

ret_t ext2_read_table(ui32_t first_block,
                      void *buf,
                      size_t elt_size,
                      ui32_t index)
{
	return ext2_read_block(first_block + (index * elt_size) / block_size,
	                       buf,
	                       elt_size,
	                       (index * elt_size) % block_size);
}

/**
 * ext2_write_table
 */

ret_t ext2_write_table(ui32_t first_block,
                       void *buf,
                       size_t elt_size,
                       ui32_t index)
{
	return ext2_write_block(first_block + (index * elt_size) / block_size,
	                        buf,
	                        elt_size,
	                        (index * elt_size) % block_size);
}

/**
 * ext2_read_inode
 */

ret_t ext2_read_inode(ui32_t inum, struct ext2_inode *inode)
{
	ui32_t bg_num = (inum - 1) / sb.s_inodes_per_group;
	ui32_t index = (inum - 1) % sb.s_inodes_per_group;
	struct ext2_bg_desc bg;

	if(ext2_read_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
	{
		return -EIO;
	}

	/*printk("inode size: %x\n", inode_size);
	printk("inode table: %x\n", bg.bg_inode_table);*/

	return ext2_read_table(bg.bg_inode_table, inode, inode_size, index);
}

/**
 * ext2_write_inode
 */

ret_t ext2_write_inode(ui32_t inum, struct ext2_inode *inode)
{
	ui32_t bg_num = (inum - 1) / sb.s_inodes_per_group;
	ui32_t index = (inum - 1) % sb.s_inodes_per_group;
	struct ext2_bg_desc bg;

	if(ext2_read_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
	{
		return -EIO;
	}

	return ext2_write_table(bg.bg_inode_table, inode, inode_size, index);
}

/**
 * ext2_get_block_coord
 */

ret_t ext2_get_block_coord(struct ext2_block_info *binfo)
{
	off_t pos = binfo->pos;

	binfo->ind = 0;
	binfo->x = 0;
	binfo->y = 0;
	binfo->z = 0;

	/** Direct block. **/

	if(pos < 12)
	{
		binfo->x = pos;
		return OK;
	}

	binfo->ind++;
	pos -= 12;

	/** Simply indirect block. **/

	if(pos < block_size / 4)
	{
		binfo->x = pos;
		return OK;
	}

	binfo->ind++;
	pos -= block_size / 4;

	/** Doubly indirect block. **/

	if(pos < block_size * block_size / 16)
	{
		binfo->x = pos / (block_size / 4);
		binfo->y = pos % (block_size / 4);
		return OK;
	}

	binfo->ind++;
	pos -= block_size * block_size / 16;

	/** Triply indirect block. **/

	if(pos < (ui64_t)block_size
	       * (ui64_t)block_size
	       * (ui64_t)block_size / 64)
	{
		ui32_t rem = pos % (block_size * block_size / 16);

		binfo->x = pos / (block_size * block_size / 16);
		binfo->y = rem / (block_size / 4);
		binfo->z = rem % (block_size / 4);
		return OK;
	}

	/** pos is too big. **/

	return -EINVAL;
}

/**
 * ext2_get_sind_data_block
 */

ret_t ext2_get_sind_data_block(ui32_t sip, ui32_t *pdata, ui32_t x)
{
	if(ext2_read_block(sip, pdata, 4, x * 4) != OK)
	{
		return -EIO;
	}

	return OK;
}

/**
 * ext2_get_dind_data_block
 */

ret_t ext2_get_dind_data_block(ui32_t dip,
                               ui32_t *pdata,
                               ui32_t *psip,
                               ui32_t x,
                               ui32_t y)
{
	if(ext2_read_block(dip, psip, 4, x * 4) != OK)
	{
		return -EIO;
	}

	if(!(*psip))
	{
		return -EINVAL;
	}

	return ext2_get_sind_data_block(*psip, pdata, y);
}

/**
 * ext2_get_tind_data_block
 */

ret_t ext2_get_tind_data_block(ui32_t tip,
                               ui32_t *pdata,
                               ui32_t *pdip,
                               ui32_t *psip,
                               ui32_t x,
                               ui32_t y,
                               ui32_t z)
{
	if(ext2_read_block(tip, pdip, 4, x * 4) != OK)
	{
		return -EIO;
	}

	if(!(*pdip))
	{
		return -EINVAL;
	}

	return ext2_get_dind_data_block(*pdip, pdata, psip, y, z);
}

/**
 * ext2_get_data_block
 */

ret_t ext2_get_data_block(struct ext2_inode *inode,
                          struct ext2_block_info *binfo)
{
	ret_t ret = ext2_get_block_coord(binfo);

	if(ret != OK)
	{
		goto end;
	}

	switch(binfo->ind)
	{
		case 0:
			binfo->data = inode->i_dps[binfo->x];
			break;

		case 1:
			binfo->ind1 = inode->i_sip;
			ret = ext2_get_sind_data_block(inode->i_sip,
			                               &binfo->data,
			                               binfo->x);
			break;

		case 2:
			binfo->ind1 = inode->i_dip;
			ret = ext2_get_dind_data_block(inode->i_dip,
			                               &binfo->data,
			                               &binfo->ind2,
			                               binfo->x,
			                               binfo->y);
			break;

		case 3:
			binfo->ind1 = inode->i_tip;
			ret = ext2_get_tind_data_block(inode->i_tip,
			                               &binfo->data,
			                               &binfo->ind2,
			                               &binfo->ind3,
			                               binfo->x,
			                               binfo->y,
			                               binfo->z);
			break;

		default:
			panic("indirection level > 3");
			break;
	}

	end:
		return ret;
}

/**
 * ext2_set_sind_data_block
 */

ret_t ext2_set_sind_data_block(ui32_t sip, ui32_t data, ui32_t x)
{
	return ext2_write_block(sip, &data, 4, x * 4);
}

/**
 * ext2_set_dind_data_block
 */

ret_t ext2_set_dind_data_block(ui32_t dip,
                               count_t *palloc_cnt,
                               ui32_t data,
                               ui32_t x,
                               ui32_t y)
{
	ui32_t sip = 0, alloc = 0;
	ui32_t zero = 0;
	ret_t error;

	if(ext2_read_block(dip, &sip, 4, x * 4) != OK)
	{
		error = EIO;
		goto fail;
	}

	if(!sip)
	{
		alloc = ext2_balloc();

		if(!alloc)
		{
			error = ENOSPC;
			goto fail;
		}

		if(ext2_zero_block(alloc) != OK
		|| ext2_write_block(dip, &alloc, 4, x * 4) != OK)
		{
			error = EIO;
			goto fail;
		}

		sip = alloc;
		*palloc_cnt = 1;
	}
	else
	{
		*palloc_cnt = 0;
	}

	if(ext2_set_sind_data_block(sip, data, y) != OK)
	{
		goto fail;
	}

	return OK;

	fail:
		if(alloc)
		{
			if(ext2_bfree(alloc) != OK)
			{
				panic("failed recovering (1)");
			}

			if(sip)
			{
				if(ext2_write_block(dip,
				                    &zero,
				                    4,
				                    x * 4) != OK)
				{
					panic("failed recovering (2)");
				}
			}
		}

		return -error;
}

/**
 * ext2_set_tind_data_block
 */

ret_t ext2_set_tind_data_block(ui32_t tip,
                               count_t *palloc_cnt,
                               ui32_t data,
                               ui32_t x,
                               ui32_t y,
                               ui32_t z)
{
	ui32_t dip = 0, alloc = 0;
	count_t alloc_cnt;
	ret_t error;
	ui32_t zero = 0;

	if(ext2_read_block(tip, &dip, 4, x * 4) != OK)
	{
		error = EIO;
		goto fail;
	}

	if(!dip)
	{
		alloc = ext2_balloc();

		if(!alloc)
		{
			error = ENOSPC;
			goto fail;
		}

		if(ext2_zero_block(alloc) != OK
		|| ext2_write_block(tip, &alloc, 4, x * 4) != OK)
		{
			error = EIO;
			goto fail;
		}

		dip = alloc;
		*palloc_cnt = 1;
	}
	else
	{
		*palloc_cnt = 0;
	}

	if(ext2_set_dind_data_block(dip, &alloc_cnt, data, y, z) != OK)
	{
		goto fail;
	}

	*palloc_cnt += alloc_cnt;

	return OK;

	fail:
		if(alloc)
		{
			if(ext2_bfree(alloc) != OK)
			{
				panic("failed recovering (1)");
			}

			if(dip)
			{
				if(ext2_write_block(tip,
				                    &zero,
				                    4,
				                    x * 4) != OK)
				{
					panic("failed recovering (2)");
				}
			}
		}

		return -error;
}

/**
 * ext2_set_data_block
 */

// Just fill in pos and data fields in block info structure.

ret_t ext2_set_data_block(struct ext2_inode *inode,
                          struct ext2_block_info *binfo,
                          count_t *palloc_cnt)
{
	ret_t ret;
	ui32_t alloc;
	count_t alloc_cnt = 0;

	*palloc_cnt = 0;

	/** Read block coordinates. **/

	ret = ext2_get_block_coord(binfo);

	if(ret != OK)
	{
		goto end;
	}

	/** Allocate a root block (if necessary). **/

	if(binfo->ind)
	{
		if(!(*(&inode->i_sip + binfo->ind - 1)))
		{
			alloc = ext2_balloc();

			//printk("allocated root block: %x\n", alloc);

			if(!alloc || ext2_zero_block(alloc) != OK)
			{
				goto end;
			}

			*(&inode->i_sip + binfo->ind - 1) = alloc;
			*palloc_cnt = 1;
		}
	}

	/** Map the data block. **/

	switch(binfo->ind)
	{
		case 0:
			inode->i_dps[binfo->x] = binfo->data;
			break;

		case 1:
			//printk("singly indirect block\n");
			ret = ext2_set_sind_data_block(inode->i_sip,
			                               binfo->data,
			                               binfo->x);
			break;

		case 2:
			ret = ext2_set_dind_data_block(inode->i_dip,
			                               &alloc_cnt,
			                               binfo->data,
			                               binfo->x,
			                               binfo->y);
			break;

		case 3:
			ret = ext2_set_tind_data_block(inode->i_tip,
			                               &alloc_cnt,
			                               binfo->data,
			                               binfo->x,
			                               binfo->y,
			                               binfo->z);
			break;
	}

	end:
		*palloc_cnt += alloc_cnt;

		if(ret != OK && alloc)
		{
			if(ext2_bfree(alloc) != OK)
			{
				panic("failed recovering");
			}

			*(&inode->i_sip + binfo->ind - 1) = 0;
		}

		return ret;
}

/**
 * ext2_read_write
 */

ssize_t ext2_read_write(ui32_t inum,
                        void *buf,
                        size_t size,
                        off_t off,
                        bool_t write)
{
	ret_t (*ext2_io)(ui32_t, void*, size_t, off_t)
	    = write ? ext2_write_block : ext2_read_block;
	ui32_t block_pos = off / block_size;
	size_t rw_bytes = 0, to_rw;
	struct ext2_inode inode;
	struct ext2_block_info binfo;
	ui32_t file_size;

	/** Fetch the inode structure. **/

	if(ext2_read_inode(inum, &inode) != OK)
	{
		return -1;
	}

	file_size = inode.i_size_low;

	/** Read/write. **/

	while(rw_bytes < size && off < file_size)
	{
		to_rw = block_size - (off % block_size);

		if(size - rw_bytes < to_rw)
		{
			to_rw = size - rw_bytes;
		}
		else if(file_size - off < to_rw)
		{
			to_rw = file_size - off;
		}

		binfo.pos = block_pos;

		if(ext2_get_data_block(&inode, &binfo) != OK)
		{
			return -1;
		}

		if(!binfo.data)
		{
			break;
		}

		if(ext2_io(binfo.data, buf + rw_bytes, to_rw, off % block_size)
		   != OK)
		{
			return -1;
		}

		block_pos++;
		rw_bytes += to_rw;
		off += to_rw;
	}

	return rw_bytes;
}

/**
 * ext2_read
 */

ssize_t ext2_read(ui32_t inum, void *buf, size_t size, off_t off)
{
	return ext2_read_write(inum, buf, size, off, 0);
}

/**
 * ext2_write
 */

ssize_t ext2_write(struct ext2_file *file, void *buf, size_t size, off_t off)
{
	return ext2_read_write(file->inum, buf, size, off, 1);
}

/**
 * ext2_bialloc
 */

ui32_t ext2_bialloc(bool_t inode)
{
	ui32_t bg_num;
	ui32_t elt, elt_index;
	struct ext2_bg_desc bg;
	ui8_t bmp_byte;

	uchar_t *elt_type = inode ? "inode" : "block"; 
	ui32_t sb_elts;
	ui32_t *psb_free_elts;
	ui32_t *pstart_elt;
	ui32_t elts_per_group;
	ui16_t *pbg_free_elts;
	ui32_t bg_bitmap;

	sb_elts = inode ? sb.s_inodes : sb.s_blocks;
	psb_free_elts = inode ? &sb.s_free_inodes : &sb.s_free_blocks;
	pstart_elt = inode ? &start_inode : &start_block;
	elts_per_group = inode
	               ? sb.s_inodes_per_group
	               : sb.s_blocks_per_group;

	for(elt = *pstart_elt; elt < sb_elts; elt++)
	{
		bg_num = elt / elts_per_group;
		elt_index = elt % elts_per_group;

		/** Read the block group descriptor if it has not been loaded
		    already. **/

		if((elt - 1) / elts_per_group != bg_num || elt == *pstart_elt)
		{
			if(ext2_read_table(bg_table, &bg, sizeof(bg), bg_num)
			   != OK)
			{
				return 0;
			}

			pbg_free_elts = inode ? &bg.bg_free_inodes
			                      : &bg.bg_free_blocks;
			bg_bitmap = inode ? bg.bg_inode_bitmap
			                  : bg.bg_block_bitmap;

			/** If there is no free element in the current group,
			    don't waste our time. **/

			if(!(*pbg_free_elts))
			{
				elt = (bg_num + 1) * elts_per_group - 1;
				continue;
			}
		}

		/** Load the next byte from the block group's elements bitmap
		    if elt_index is a multiple of 8 or elt is the first
		    considered element. **/

		if(!(elt_index % 8) || elt == *pstart_elt)
		{
			if(ext2_read_table(bg_bitmap,
			                   &bmp_byte,
			                   1,
			                   elt_index / 8) != OK)
			{
				return 0;
			}

			/** If the current bitmap byte shows that no element
			    is free, switch to the next bitmap byte. **/

			if(bmp_byte == 0xff)
			{
				elt = (elt & ~7) + 7;
				continue;
			}
		}

		if(!(bmp_byte & (1 << (elt_index % 8))))
		{
			/** We have found a free element! Mark it as used. **/

			bmp_byte |= (1 << (elt_index % 8));

			if(ext2_write_table(bg_bitmap,
			                    &bmp_byte,
			                    1,
			                    elt_index / 8) != OK)
			{
				return 0;
			}

			if(!(*pbg_free_elts))
			{
				panic("bg free %ss field corrupted", elt_type);
			}

			(*pbg_free_elts)--;

			if(ext2_write_table(bg_table, &bg, sizeof(bg), bg_num)
			   != OK)
			{
				bmp_byte &= ~(1 << (elt_index % 8));

				if(ext2_write_table(bg_bitmap,
					            &bmp_byte,
					            1,
					            elt_index / 8) != OK)
				{
					panic("unrecoverable error");
				}

				return 0;
			}

			if(!(*psb_free_elts))
			{
				panic("sb free %ss field corrupted", elt_type);
			}

			(*psb_free_elts)--;
			*pstart_elt = elt + 1;

			return (inode || block_size == 1024) ? (elt + 1) : elt;
		}
	}

	return 0;
}

/**
 * ext2_balloc
 */

ui32_t ext2_balloc()
{
	return ext2_bialloc(0);
}

/**
 * ext2_ialloc
 */

ui32_t ext2_ialloc()
{
	return ext2_bialloc(1);
}

/**
 * ext2_bifree
 */

ret_t ext2_bifree(bool_t inode, ui32_t elt)
{
	ui32_t bg_num;
	struct ext2_bg_desc bg;
	ui32_t elt_index;
	ui8_t bmp_byte;

	ui32_t elts_per_group;
	ui32_t *psb_free_elts;
	ui32_t bg_bitmap;
	ui16_t *pbg_free_elts;
	ui32_t *pstart_elt;
	uchar_t *elt_type = inode ? "inode" : "block";

	if(inode || block_size == 1024)
	{
		elt--;
	}

	elts_per_group = inode ? sb.s_inodes_per_group
	                       : sb.s_blocks_per_group;
	psb_free_elts = inode ? &sb.s_free_inodes
	                      : &sb.s_free_blocks;
	pstart_elt = inode ? &start_inode : &start_block;

	bg_num = elt / elts_per_group;
	elt_index = elt % elts_per_group;

	/** Get block group. **/

	if(ext2_read_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
	{
		return -EIO;
	}

	bg_bitmap = inode ? bg.bg_inode_bitmap : bg.bg_block_bitmap;
	pbg_free_elts = inode ? &bg.bg_free_inodes : &bg.bg_free_blocks;

	/** Read bitmap. **/

	if(ext2_read_table(bg_bitmap, &bmp_byte, 1, elt_index / 8) != OK)
	{
		return -EIO;
	}

	/** Check whether the element is already free. **/

	if(!(bmp_byte & (1 << (elt_index % 8))))
	{
		panic("%s %x is already free", elt_type, elt + 1);
	}

	/** Free. **/

	bmp_byte &= ~(1 << (elt_index % 8));

	if(ext2_write_table(bg_bitmap, &bmp_byte, 1, elt_index / 8) != OK)
	{
		return -EIO;
	}

	/** Update block group descriptor and superblock. **/

	(*pbg_free_elts)++;

	if(ext2_write_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
	{
		return -EIO;
	}

	(*psb_free_elts)++;

	/** Update *pstart_elt. **/

	if(elt < *pstart_elt)
	{
		//printk("updating first free %s to %x\n", elt_type, elt);
		*pstart_elt = elt; 
	}

	return OK;
}

/**
 * ext2_bfree
 */

ret_t ext2_bfree(ui32_t block)
{
	return ext2_bifree(0, block);
}

/**
 * ext2_ifree
 */

ret_t ext2_ifree(ui32_t inum)
{
	return ext2_bifree(1, inum);
}

/**
 * ext2_truncate
 */

ret_t ext2_truncate(struct ext2_file *file, off_t len)
{
	ui32_t file_size;
	ui32_t to_trunc;
	struct ext2_block_info binfo;
	ret_t ret = OK;

	if(len < 0)
	{
		return -EINVAL;
	}

	file_size = file->inode.i_size_low;

	while(file_size > len)
	{
		binfo.pos = (file_size - 1) / block_size;
		to_trunc = (file_size % block_size)
		         ? (file_size % block_size)
		         : block_size;

		if(file_size - len < to_trunc)
		{
			to_trunc = file_size - len;
		}

		/** Free the last block of the file if necessary (don't forget
		    indirect blocks). **/

		if((file_size - to_trunc - 1) / block_size != binfo.pos)
		{
			ret = ext2_get_data_block(&file->inode, &binfo);

			if(ret != OK)
			{
				#ifdef DEBUG_EXT2
				printk("ext2 trunc error 2\n");
				#endif
				goto end;
			}

			ret = ext2_bfree(binfo.data);

			if(ret != OK)
			{
				#ifdef DEBUG_EXT2
				printk("ext2 trunc error 3\n");
				#endif
				goto end;
			}

			file->inode.i_secs -= block_size / 512;

			if(binfo.ind == 3)
			{
				ret = ext2_set_sind_data_block(binfo.ind3,
				                               0,
				                               binfo.z);

				if(ret != OK)
				{
					#ifdef DEBUG_EXT2
					printk("ext2 trunc error 4\n");
					#endif
					goto end;
				}

				if(!binfo.z)
				{
					ret = ext2_bfree(binfo.ind3);

					if(ret != OK)
					{
						#ifdef DEBUG_EXT2
						printk("ext2 trunc error 5\n");
						#endif
						goto end;
					}

					file->inode.i_secs -= block_size / 512;
				}
			}

			if((binfo.ind == 3 && !binfo.z) || binfo.ind == 2)
			{
				ret = ext2_set_sind_data_block(binfo.ind2,
				                               0,
				                               binfo.y);

				if(ret != OK)
				{
					#ifdef DEBUG_EXT2
					printk("ext2 trunc error 6\n");
					#endif
					goto end;
				}

				if(!binfo.y)
				{
					ret = ext2_bfree(binfo.ind2);

					if(ret != OK)
					{
						#ifdef DEBUG_EXT2
						printk("ext2 trunc error 7\n");
						#endif
						goto end;
					}

					file->inode.i_secs -= block_size / 512;
				}
			}

			if((binfo.ind == 3 && !binfo.y && !binfo.z)
			|| (binfo.ind == 2 && !binfo.y)
			|| (binfo.ind == 1))
			{
				ret = ext2_set_sind_data_block(binfo.ind1,
				                               0,
				                               binfo.x);

				if(ret != OK)
				{
					#ifdef DEBUG_EXT2
					printk("ext2 trunc error 8\n");
					#endif
					goto end;
				}

				if(!binfo.x)
				{
					ret = ext2_bfree(binfo.ind1);

					if(ret != OK)
					{
						#ifdef DEBUG_EXT2
						printk("ext2 trunc error 9\n");
						#endif
						goto end;
					}

					file->inode.i_secs -= block_size / 512;
					*(&file->inode.i_sip + binfo.ind - 1) = 0;
				}
			}

			if(binfo.ind == 0)
			{
				file->inode.i_dps[binfo.x] = 0;
			}
		}

		file_size -= to_trunc;
	}

	end:
		file->inode.i_size_low = file_size;

		if(ext2_write_inode(file->inum, &file->inode) != OK)
		{
			#ifdef DEBUG_EXT2
			printk("ext2 trunc error 10\n");
			#endif
		}

		return ret;
}

/**
 * ext2_append
 */

ret_t ext2_append(struct ext2_file *file, off_t len)
{
	ui32_t old_file_size, new_file_size, file_size;
	struct ext2_block_info binfo;
	ui32_t to_app;
	ui32_t alloc = 0;
	count_t alloc_cnt;
	ret_t ret = OK;

	old_file_size = file_size = file->inode.i_size_low;
	new_file_size = old_file_size + len;

	while(file_size < new_file_size)
	{
		to_app = (new_file_size - file_size < block_size)
		       ? (new_file_size - file_size)
		       : block_size;
		binfo.pos = (file_size + to_app - 1) / block_size;

		ret = ext2_get_data_block(&file->inode, &binfo);

		if((file_size - 1) / block_size != binfo.pos)
		{
			alloc = ext2_balloc();

			if(!alloc)
			{
				ret = -ENOSPC;
				goto end;
			}

			binfo.data = alloc;
			ret = ext2_set_data_block(&file->inode, &binfo, &alloc_cnt);

			if(ret != OK)
			{
				goto end;
			}

			file->inode.i_secs += (1 + alloc_cnt) * block_size / 512;
			alloc = 0;
		}

		file_size += to_app;
	}

	end:
		file->inode.i_size_low = file_size;

		if(ext2_write_inode(file->inum, &file->inode) != OK)
		{
			return -EIO;
		}

		if(ret != OK)
		{
			if(alloc)
			{
				if(ext2_bfree(alloc) != OK)
				{
					panic("failed recovering (1)");
				}
			}

			if(ext2_truncate(file, old_file_size) != OK)
			{
				panic("failed recovering (2)");
			}
		}

		return ret;
}

/**
 * ext2_link
 */

ret_t ext2_link(struct ext2_file *dir, struct ext2_file *file, uchar_t *name)
{
	size_t name_len;
	ret_t ret;
	struct ext2_dent dent, dent2;
	off_t off = 0;
	ssize_t read_bytes;
	ui16_t new_dent_size;
	bool_t found = 0;

	/** Check name. **/

	name_len = strlen(name);

	if(name_len > 255)
	{
		return -ENAMETOOLONG;
	}

	if(strchr(name, '/'))
	{
		return -EINVAL;
	}

	#ifdef DEBUG_EXT2
	printk("linking %s (%x) in %x\n", name, file->inum, dir->inum);
	#endif

	/** Try to find a free directory entry. **/

	do
	{
		read_bytes = ext2_read(dir->inum, &dent, sizeof(dent), off);

		#ifdef DEBUG_EXT2
		printk("read_bytes: %x\n", read_bytes);
		#endif

		if(read_bytes == -1)
		{
			return -EIO;
		}

		if(read_bytes)
		{
			#ifdef DEBUG_EXT2
			printk("d_name: %s | d_inode: %x\n",
			       dent.d_name, dent.d_inode);
			#endif

			if(!dent.d_inode && dent.d_size > (name_len + 8))
			{
				found = 1;
				dent.d_inode = file->inum;
				dent.d_name_len = (ui8_t)name_len;
				dent.d_type = 0;
				strcpy(dent.d_name, name);

				/** Cut in 2 if possible. **/

				new_dent_size
				 = ((ui16_t)name_len + 9 + 3) & ~3;

				if(dent.d_size - new_dent_size
				>= EXT2_MIN_DIRENT_SIZE)
				{
					dent2.d_inode = 0;
					dent2.d_name_len = 0;
					dent2.d_size = dent.d_size
					             - new_dent_size;
					dent2.d_type = 0;
					dent.d_size = new_dent_size;

					#ifdef DEBUG_EXT2
					printk("dent sz: %x | dent2 sz: %x\n",
					       dent.d_size, dent2.d_size);
					#endif

					if(ext2_write(dir,
					              &dent2,
					              sizeof(dent2),
					              off + dent.d_size) == -1)
					{
						return -EIO;
					}
				}

				#ifdef DEBUG_EXT2
				printk("at offset %x\n", off);
				#endif

				if(ext2_write(dir,
				              &dent,
				              dent.d_size,
				              off) == -1)
				{
					return -EIO;
				}

				break;
			}
			/*else
			{
				printk("\tname: %s\n", dent.d_name);
			}*/

			off += dent.d_size;
		}
	} while(read_bytes);

	/** If no entry was found, append one to the end of the directory. **/

	if(!found)
	{
		dent.d_inode = file->inum;
		dent.d_size = ((ui16_t)name_len + 9 + 3) & ~3;
		dent.d_name_len = (ui8_t)name_len;
		dent.d_type = 0;
		strcpy(dent.d_name, name);

		/** Append a block to the directory. **/

		if((ret = ext2_append(dir, block_size)) != OK)
		{
			return ret;
		}

		/** Write the new dirent at the beginning of the block. **/

		if(ext2_write(dir, &dent, dent.d_size, off) == -1)
		{
			return -EIO;
		}

		/** Write a null dirent after this one. **/

		dent2.d_inode = 0;
		dent2.d_size = block_size - dent.d_size;

		if(ext2_write(dir,
		              &dent2,
		              sizeof(dent2),
		              off + dent.d_size) == -1)
		{
			return -EIO;
		}
	}

	/** Update number of links and rewrite inode to disk. **/

	file->inode.i_links++;

	if((ret = ext2_write_inode(file->inum, &file->inode)) != OK)
	{
		return ret;
	}

	return OK;
}

/**
 * ext2_find_dent
 */

ret_t ext2_find_dent(ui32_t dir_inum,
                     uchar_t *name,
                     struct ext2_dent_info *dent_info)
{
	ssize_t read_bytes;

	dent_info->off = 0;

	while((read_bytes = ext2_read(dir_inum,
	                              &dent_info->dent,
	                              sizeof(dent_info->dent),
	                              dent_info->off)) > 0)
	{
		if(dent_info->dent.d_inode
		&& strlen(name) == dent_info->dent.d_name_len
		&& !strncmp(dent_info->dent.d_name,
		            name,
		            (size_t)dent_info->dent.d_name_len))
		{
			break;
		}

		dent_info->off += dent_info->dent.d_size;
	}

	if(read_bytes == -1)
	{
		return -EIO;
	}
	else if(!read_bytes)
	{
		return -ENOENT;
	}

	return OK;
}

/**
 * ext2_dir_empty
 */

bool_t ext2_dir_empty(ui32_t dir_inum)
{
	struct ext2_dent dent;
	ssize_t read_bytes;
	off_t off = 0;
	count_t inodes = 0;

	while((read_bytes = ext2_read(dir_inum, &dent, sizeof(dent), off)) > 0)
	{
		if(dent.d_inode)
		{
			inodes++;
		}

		off += dent.d_size;
	}

	/** In case the reading failed, assume the directory was not empty. **/

	if(read_bytes == -1)
	{
		return 0;
	}

	if(inodes < 2)
	{
		panic("directory %x contains less than 2 elements", dir_inum);
	}

	return inodes == 2;
}

/**
 * ext2_unlink
 */

ret_t ext2_unlink(struct ext2_file *dir, struct ext2_file *file, uchar_t *name)
{
	struct ext2_dent_info dent_info;
	ret_t ret;

	/** Find the directory entry. **/

	ret = ext2_find_dent(dir->inum, name, &dent_info);

	if(ret != OK)
	{
		return ret;
	}

	if(dent_info.dent.d_inode != file->inum)
	{
		return -EINVAL;
	}

	/** Free the directory entry. **/

	dent_info.dent.d_inode = 0;

	if(ext2_write(dir,
	              &dent_info.dent,
	              (size_t)min(dent_info.dent.d_size,
	                          sizeof(dent_info.dent)),
	              dent_info.off) == -1)
	{
		return -EIO;
	}

	/** Decrement the number of links pointing to the file. **/

	file->inode.i_links--;
	file->inode.i_dtime = /*time(NULL)*/0;

	if((ret = ext2_write_inode(file->inum, &file->inode)) != OK)
	{
		return ret;
	}

	/** Do not forget to truncate the file and free its inode if there is
	    no more hard links pointing to it. **/

	if(!file->inode.i_links)
	{
		if((ret = ext2_remove(file->inum)) != OK)
		{
			return ret;
		}
	}

	return OK;
}

/**
 * ext2_create
 */

ui32_t ext2_create(struct ext2_file *parent,
                   ui16_t type_perm,
                   struct ext2_file *file)
{
	time_t t = /*time(NULL)*/0;
	ret_t ret;

	file->inode.i_type_perm = type_perm;
	file->inode.i_uid = 0;
	file->inode.i_size_low = 0;
	file->inode.i_atime = t;
	file->inode.i_ctime = t;
	file->inode.i_mtime = t;
	file->inode.i_dtime = 0;
	file->inode.i_gid = 0;
	file->inode.i_links = 0;
	file->inode.i_secs = 0;
	file->inode.i_flags = 0;
	file->inode.i_os1 = 0;
	memset(&file->inode.i_dps, 0, sizeof(ui32_t) * 12);
	file->inode.i_sip = 0;
	file->inode.i_dip = 0;
	file->inode.i_tip = 0;
	file->inode.i_gen_num = 0;
	file->inode.i_ext_attr_block = 0;
	file->inode.i_size_high = 0;
	file->inode.i_frag_block = 0;
	memset(&file->inode.i_os2, 0, sizeof(ui8_t) * 12);

	if(!(type_perm & EXT2_DIR) && !(type_perm & EXT2_REG_FILE))
	{
		ret = -EINVAL;
		goto fail1;
	}

	/** Allocate an inode number and write the inode structure to the
	    disk. **/

	file->inum = ext2_ialloc();

	if(!file->inum)
	{
		ret = -ENOSPC;
		goto fail1;
	}

	if((ret = ext2_write_inode(file->inum, &file->inode)) != OK)
	{
		goto fail2;
	}

	/** If we have a directory, write the first two entries '.' and
	    '..'. Also update bg_dirs in the block group of the new inode. **/

	if(type_perm & EXT2_DIR)
	{
		ui32_t bg_num = (file->inum - 1) / sb.s_inodes_per_group;
		struct ext2_bg_desc bg;

		if((ret = ext2_link(file, file, ".")) != OK
		|| (ret = ext2_link(file, parent, "..")) != OK)
		{
			goto fail3;
		}

		if((ret = ext2_read_table(bg_table, &bg, sizeof(bg), bg_num))
		   != OK)
		{
			goto fail3;
		}

		bg.bg_dirs++;

		if((ret = ext2_write_table(bg_table, &bg, sizeof(bg), bg_num))
		   != OK)
		{
			goto fail3;
		}
	}

	return OK;

	fail3:
		if(ext2_remove(file->inum) != OK)
		{
			panic("failed recovering (3)");
		}

		goto fail1;
	fail2:
		if(ext2_ifree(file->inum) != OK)
		{
			panic("failed recovering (4)");
		}
	fail1:
		return ret;
}

/**
 * ext2_remove
 */

ret_t ext2_remove(ui32_t inum)
{
	ret_t ret;
	struct ext2_file file;
	struct ext2_bg_desc bg;
	ui32_t bg_num;

	file.inum = inum;

	if((ret = ext2_read_inode(inum, &file.inode)) != OK)
	{
		goto end;
	}

	if((ret = ext2_truncate(&file, 0)) != OK)
	{
		goto end;
	}

	if((ret = ext2_ifree(inum)) != OK)
	{
		goto end;
	}

	if(file.inode.i_type_perm & EXT2_DIR)
	{
		bg_num = (inum - 1) / sb.s_inodes_per_group;

		if(ext2_read_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
		{
			return -EIO;
		}

		/** Get block group. **/

		if(ext2_read_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
		{
			return -EIO;
		}

		/** Update block group descriptor. **/

		bg.bg_dirs--;

		if(ext2_write_table(bg_table, &bg, sizeof(bg), bg_num) != OK)
		{
			return -EIO;
		}
	}

	end:
		return ret;
}

/**
 * ext2_make_file
 */

ret_t ext2_make_file(ui32_t inum, struct ext2_file *dest)
{
	ret_t ret;

	dest->inum = inum;

	if((ret = ext2_read_inode(inum, &dest->inode)) != OK)
	{
		return ret;
	}

	return OK;
}
