#ifndef _FAT32_H_
#define _FAT32_H_

#include <kernel/types.h>

/** FAT32 superblock. **/

struct fat32_sb
{
	// Bios parameter block

	ui8_t jump[3];
	ui8_t oem_id[8];
	ui16_t bytes_per_sec;
	ui8_t sec_per_cluster;
	ui16_t res_sec_cnt;
	ui8_t fat_cnt;
	ui16_t root_dent_cnt;
	ui16_t sec_cnt; // unused
	ui8_t media_desc_type;
	ui16_t _sec_per_fat; // unused
	ui16_t sec_per_track;
	ui16_t head_cnt;
	ui32_t hidden_sec_cnt;
	ui32_t large_sec_cnt;

	// Extended boot record

	ui32_t sec_per_fat;
	ui16_t flags;
	ui16_t fat_ver;
	ui32_t root_dir_cluster;
	ui16_t fsinfo_sec;
	ui16_t backup_bs_cluster; // bs: boot sector
	ui8_t reserved[12];
	ui8_t drive_num;
	ui8_t reserved2;
	ui8_t signature; // 0x28 or 0x29
	ui8_t vol_id[4];
	ui8_t vol_label[11];
	ui8_t sys_id[8];
	ui8_t boot_code[420];
	ui16_t boot_signature;
} __attribute__((packed));

/** FS information sector. **/

struct fat32_fsinfo
{
	ui8_t sign[4]; // "RRaA"
	ui8_t reserved[480];
	ui8_t sign2[4]; // "rrAa"
	ui32_t free_clusters;
	ui32_t alloc_clusters;
	ui8_t reserved2[12];
	ui8_t sign3[4]; // 0x00, 0x00, 0x55, 0xAA
} __attribute__((packed));

/** 8.3 Standard directory entry. **/

struct fat32_dent
{
	ui8_t file_name[11];
	ui8_t attr;
	ui8_t reserved;
	ui8_t creat_time_tenth_sec;
	ui16_t creat_time;
	ui16_t creat_date;
	ui16_t last_acc_date;
	ui16_t first_cluster_high;
	ui16_t last_mod_time;
	ui16_t last_mod_date;
	ui16_t first_cluster_low;
	ui32_t file_size;
} __attribute__((packed));

/** A convenient structure holding the information linked to a file. **/

struct fat32_file
{
	ui32_t dent_cluster;
	off_t dent_off; /* offset of the directory entry inside the directory
	                   (not in bytes, but in directory entries, a directory
	                   entry being 32 bytes) */
	struct fat32_dent dent;
};

/** File attributes. **/

#define FAT32_ATTR_READ_ONLY	0x01
#define FAT32_ATTR_HIDDEN	0x02
#define FAT32_ATTR_SYSTEM	0x04
#define FAT32_ATTR_VOLUME_ID	0x08
#define FAT32_ATTR_DIRECTORY	0x10
#define FAT32_ATTR_ARCHIVE	0x20

/** Global variable: root directory (initialized by fat32_init). **/

#ifdef _FAT32_C_
struct fat32_file root_dir;
#else
extern struct fat32_file root_dir;
#endif

/** Functions. **/

void fat32_init();
/****************************************************************/
ret_t fat32_sync();
/****************************************************************/
ret_t fat32_get_cluster_desc(ui32_t cluster, ui32_t *cluster_desc);
/****************************************************************/
ret_t fat32_set_cluster_desc(ui32_t cluster, ui32_t cluster_desc);
/****************************************************************/
ui32_t fat32_alloc_clusters(count_t count);
/****************************************************************/
ret_t fat32_free_clusters(ui32_t first);
/****************************************************************/
ui32_t fat32_get_cluster_n(ui32_t first, ui32_t n);
/****************************************************************/
ui32_t fat32_get_last_cluster(ui32_t first);
/****************************************************************/
ret_t fat32_read_cluster(ui32_t cluster, void *buf, size_t size, off_t off);
/****************************************************************/
ret_t fat32_write_cluster(ui32_t cluster, void *buf, size_t size, off_t off);
/****************************************************************/
ssize_t fat32_read_write(ui32_t first_cluster,
                         void *buf,
                         size_t size,
                         off_t off,
                         bool_t write);
/****************************************************************/
ssize_t fat32_read(struct fat32_file *file, void *buf, size_t size, off_t off);
/****************************************************************/
ssize_t fat32_write(struct fat32_file *file,
                    void *buf,
                    size_t size,
                    off_t off);
/****************************************************************/
ret_t fat32_append(struct fat32_file *file, size_t bytes);
/****************************************************************/
ret_t fat32_truncate(struct fat32_file *file, size_t size);
/****************************************************************/
ret_t fat32_convert_name(uchar_t *src, uchar_t *dest);
/****************************************************************/
ret_t fat32_look_for_dent(struct fat32_file *dir,
                          struct fat32_file *file,
                          uchar_t *name);
/****************************************************************/
count_t fat32_count_dent(struct fat32_file *dir);
/****************************************************************/
ret_t fat32_create(struct fat32_file *dir,
                   struct fat32_file *file,
                   uchar_t *name,
                   bool_t is_dir);
/****************************************************************/
ret_t fat32_remove(struct fat32_file *file);
/****************************************************************/
ret_t fat32_link(struct fat32_file *src, struct fat32_file *dest);
/****************************************************************/
ret_t fat32_unlink(struct fat32_file *file);

#endif
