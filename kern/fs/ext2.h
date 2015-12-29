#ifndef _EXT2_H_
#define _EXT2_H_

#include <kernel/types.h>

/** Superblock. **/

struct ext2_sb
{
	/** Basic fields. **/

	ui32_t s_inodes;
	ui32_t s_blocks;
	ui32_t s_res_blocks;
	ui32_t s_free_blocks;
	ui32_t s_free_inodes;
	ui32_t s_sb_block;
	ui32_t s_log_block_size; // in fact log_10(size) - 10
	ui32_t s_log_frag_size;
	ui32_t s_blocks_per_group;
	ui32_t s_frags_per_group;
	ui32_t s_inodes_per_group;
	ui32_t s_last_mount;
	ui32_t s_last_write;
	ui16_t s_mnt_cnt;
	ui16_t s_max_mnt_cnt;
	ui16_t s_sig;
	ui16_t s_state;
	ui16_t s_ehandling;
	ui16_t s_ver_min;
	ui32_t s_last_check;
	ui32_t s_check_interval;
	ui32_t s_osid;
	ui32_t s_ver_maj;
	ui16_t s_res_uid;
	ui16_t s_res_gid;

	/** Extended fields. **/

	ui32_t s_first_free_inode;
	ui16_t s_inode_size;
	ui16_t s_block_group;
	ui32_t s_features_opt;
	ui32_t s_features_req;
	ui32_t s_features_ro;
	ui8_t s_fs_id[16];
	ui8_t s_vol_name[16];
	ui8_t s_last_mnt_path[64];
	ui32_t s_comp_algo;
	ui8_t s_file_prealloc;
	ui8_t s_dir_prealloc;
	ui16_t s_unused;
	ui32_t s_journal_inode;
	ui32_t s_journal_device;
	ui32_t s_orphan_head;
} __attribute__((packed));

/** Block group descriptor. **/

struct ext2_bg_desc
{
	ui32_t bg_block_bitmap;
	ui32_t bg_inode_bitmap;
	ui32_t bg_inode_table;
	ui16_t bg_free_blocks;
	ui16_t bg_free_inodes;
	ui16_t bg_dirs;
	ui16_t bg_pad;
	ui8_t bg_reserved[12];
} __attribute__((packed));

/** Inode. **/

struct ext2_inode
{
	ui16_t i_type_perm;
	ui16_t i_uid;
	ui32_t i_size_low;
	ui32_t i_atime;
	ui32_t i_ctime;
	ui32_t i_mtime;
	ui32_t i_dtime;
	ui16_t i_gid;
	ui16_t i_links; // hard links only
	ui32_t i_secs;
	ui32_t i_flags;
	ui32_t i_os1;
	ui32_t i_dps[12]; // direct pointers
	ui32_t i_sip, i_dip, i_tip; // indirect pointers
	ui32_t i_gen_num;
	ui32_t i_ext_attr_block; // (only if version >= 1, unused otherwise)
	ui32_t i_size_high; // (in version >= 1 - directory ACL if it's a dir.)
	ui32_t i_frag_block;
	ui8_t i_os2[12];
} __attribute__((packed));

/** Directory entry. **/

struct ext2_dent
{
	ui32_t d_inode;
	ui16_t d_size;
	ui8_t d_name_len;
	 /** Only if feature bit 'directory entries have type byte' is set **/
	ui8_t d_type;
	uchar_t d_name[256];
} __attribute__((packed));

/** Block information. **/

struct ext2_block_info
{
	off_t pos;
	ui8_t ind;
	ui32_t x, y, z;
	ui32_t ind1, ind2, ind3, data;
};

/** Directory entry information. **/

struct ext2_dent_info
{
	off_t off;
	struct ext2_dent dent;
};

/** File. **/

struct ext2_file
{
	ui32_t inum;
	struct ext2_inode inode;
};

/** Inode types. **/

#define EXT2_DIR	0x4000
#define EXT2_REG_FILE	0x8000

/** Constants. **/

#define EXT2_MIN_DIRENT_SIZE	64 // should be a power of two less than 1024

/** Global variables. **/
#ifdef _EXT2_C_
struct ext2_sb sb;
#else
extern struct ext2_sb sb;
#endif


/** Functions. **/

void ext2_init();
/****************************************************************/
ret_t ext2_sync();
/****************************************************************/
ret_t ext2_read_block(ui32_t block, void *buf, size_t size, off_t off);
/****************************************************************/
ret_t ext2_write_block(ui32_t block, void *buf, size_t size, off_t off);
/****************************************************************/
ret_t ext2_zero_block(ui32_t block);
/****************************************************************/
ret_t ext2_read_table(ui32_t first_block,
                      void *buf,
                      size_t elt_size,
                      ui32_t index);
/****************************************************************/
ret_t ext2_write_table(ui32_t first_block,
                       void *buf,
                       size_t elt_size,
                       ui32_t index);
/****************************************************************/
ret_t ext2_read_inode(ui32_t inum, struct ext2_inode *inode);
/****************************************************************/
ret_t ext2_write_inode(ui32_t inum, struct ext2_inode *inode);
/****************************************************************/
ret_t ext2_get_sind_data_block(ui32_t sip, ui32_t *pdata, ui32_t x);
/****************************************************************/
ret_t ext2_get_dind_data_block(ui32_t dip,
                               ui32_t *pdata,
                               ui32_t *psip,
                               ui32_t x,
                               ui32_t y);
/****************************************************************/
ret_t ext2_get_tind_data_block(ui32_t tip,
                               ui32_t *pdata,
                               ui32_t *pdip,
                               ui32_t *psip,
                               ui32_t x,
                               ui32_t y,
                               ui32_t z);
/****************************************************************/
ret_t ext2_get_data_block(struct ext2_inode *inode,
                          struct ext2_block_info *binfo);
/****************************************************************/
ret_t ext2_get_block_coord(struct ext2_block_info *binfo);
/****************************************************************/
ret_t ext2_set_sind_data_block(ui32_t sip, ui32_t data, ui32_t x);
/****************************************************************/
ret_t ext2_set_dind_data_block(ui32_t dip,
                               count_t *palloc_cnt,
                               ui32_t data,
                               ui32_t x,
                               ui32_t y);
/****************************************************************/
ret_t ext2_set_tind_data_block(ui32_t tip,
                               count_t *palloc_cnt,
                               ui32_t data,
                               ui32_t x,
                               ui32_t y,
                               ui32_t z);
/****************************************************************/
ret_t ext2_set_data_block(struct ext2_inode *inode,
                          struct ext2_block_info *binfo,
                          count_t *palloc_cnt);
/****************************************************************/
ssize_t ext2_read_write(ui32_t inum,
                        void *buf,
                        size_t size,
                        off_t off,
                        bool_t write);
/****************************************************************/
ssize_t ext2_read(ui32_t inum, void *buf, size_t size, off_t off);
/****************************************************************/
ssize_t ext2_write(struct ext2_file *file, void *buf, size_t size, off_t off);
/****************************************************************/
ui32_t ext2_bialloc(bool_t inode);
/****************************************************************/
ui32_t ext2_balloc();
/****************************************************************/
ui32_t ext2_ialloc();
/****************************************************************/
ret_t ext2_bifree(bool_t inode, ui32_t elt);
/****************************************************************/
ret_t ext2_bfree(ui32_t block);
/****************************************************************/
ret_t ext2_ifree(ui32_t inum);
/****************************************************************/
ret_t ext2_truncate(struct ext2_file *file, off_t len);
/****************************************************************/
ret_t ext2_append(struct ext2_file *file, off_t len);
/****************************************************************/
ret_t ext2_link(struct ext2_file *dir, struct ext2_file *file, uchar_t *name);
/****************************************************************/
ret_t ext2_find_dent(ui32_t dir_inum,
                     uchar_t *name,
                     struct ext2_dent_info *dent_info);
/****************************************************************/
bool_t ext2_dir_empty(ui32_t dir_inum);
/****************************************************************/
ret_t ext2_unlink(struct ext2_file *dir,
                  struct ext2_file *file,
                  uchar_t *name);
/****************************************************************/
ui32_t ext2_create(struct ext2_file *parent,
                   ui16_t type_perm,
                   struct ext2_file *file);
/****************************************************************/
ret_t ext2_remove(ui32_t inum);
/****************************************************************/
ret_t ext2_make_file(ui32_t inum, struct ext2_file *dest);

#endif
