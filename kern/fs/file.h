#ifndef _FILE_H_
#define _FILE_H_

#include <config.h>
#include <fs/ext2.h>
#include <fs/fifo.h>
#include <kernel/types.h>
#include <net/tcp.h>

/** Types **/

typedef enum
{
	FS_NOFS = 1,
	FS_EXT2 = 2,
	FS_DEVFS = 3,
	FS_TCPSOCKFS = 4,
	FS_PIPEFS = 5
} fs_t;
typedef struct
{
	ui32_t fds[(NR_FILDES + 31) / 32];
} fd_set;
typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;
typedef struct { int val[2]; } fsid_t;

/** File structure. **/

struct file
{
	fs_t fs;
	bool_t used;
	count_t ref_cnt;
	union
	{
		struct ext2_file ext2_file;
		si32_t tcp_sock_id;
		si32_t pipe_id;
	} data;
};

/** File descriptor structure. **/

struct fildes
{
	bool_t used;
	count_t ref_cnt;
	off_t off;
	ino_t inum;
};

/** stat structure. **/

struct stat
{
	ui16_t st_dev;
	ui16_t st_ino;
	ui32_t st_mode;
	ui16_t st_nlink;
	ui16_t st_uid;
	ui16_t st_gid;
	ui16_t st_rdev;
	off_t st_size;
	time_t st_atime;
	si32_t st_spare1;
	time_t st_mtime;
	si32_t st_spare2;
	time_t st_ctime;
	si32_t st_spare3;
	si32_t st_blksize;
	si32_t st_blocks;
	si32_t st_spare4[2];
} __attribute__((packed));

/** statfs structure. **/

struct statfs
{
	long f_type;
	long f_bsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsid_t f_fsid;
	long f_namelen;
	long f_frsize;
	long f_spare[5];
};

/** sockaddr structure. **/

struct sockaddr
{
	ui16_t sa_family;
	ui8_t sa_data[14];
} __attribute__((packed));

/** in_addr structure. **/

struct in_addr
{
	ui32_t s_addr;
};

/** sockaddr_in structure. **/

struct sockaddr_in
{
	ui16_t sin_family;
	ui16_t sin_port;
	struct in_addr sin_addr;
	ui8_t sin_zero[8];
} __attribute__((packed));

#define FD_ISSET(fd, fdset)	((fdset)->fds[fd / 32] & (1 << (fd % 32)))
#define FD_CLR(fd, fdset)	(fdset)->fds[fd / 32] &= ~(1 << (fd % 32))
#define FD_SET(fd, fdset)	(fdset)->fds[fd / 32] |= (1 << (fd % 32))

/** open syscall flags (as defined in Newlib) **/

#define O_RDONLY	0x00000
#define O_WRONLY	0x00001
#define O_RDWR		0x00002
#define O_APPEND	0x00008
#define O_CREAT		0x00200
#define O_TRUNC		0x00400
#define O_EXCL		0x00800
#define O_NONBLOCK	0x04000
#define O_CLOEXEC	0x40000

/** Flags for field st_mode in stat structure. **/

#define S_IFSOCK	0140000
#define S_IFLNK		0120000
#define S_IFREG		0100000
#define S_IFBLK		0060000
#define S_IFDIR		0040000
#define S_IFCHR		0020000
#define S_IFIFO		0010000

/** Seek directions. **/

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

/** fcntl commands. **/

#define F_DUPFD	0
#define F_GETFD	1
#define F_SETFD	2
#define F_GETFL	3
#define F_SETFL	4
#define F_SETLK	8

/** Flag for command SETFD. **/

#define FD_CLOEXEC	1

/** Socket domains. **/

#define AF_INET	2

/** Socket types. **/

#define SOCK_STREAM	1
#define SOCK_DGRAM	2
#define SOCK_RAW	3

/** Socket protocols. **/

#define PF_INET	AF_INET

/** Global file table. **/

#ifdef _FILE_C_
struct file file_tab[NR_FILES] = {
	{ // tty
		.fs = FS_DEVFS,
		.used = 1,
		.ref_cnt = 1
	},
	{ // disk
		.fs = FS_DEVFS,
		.used = 1,
		.ref_cnt = 1
	},
	[2 ... (NR_FILES - 1)] = {
		.fs = FS_NOFS,
		.used = 0,
		.ref_cnt = 0
	}
};
struct fildes fildes_tab[NR_FILDES] = {
	{ // tty
		.used = 1,
		.ref_cnt = 1,
		.off = 0,
		.inum = 1
	},
	[1 ... (NR_FILDES - 1)] = {
		.used = 0
	}
};
#else
extern struct file file_tab[];
extern struct fildes fildes_tab[];
#endif
extern struct pipe pipe_tab[];

/** Functions. **/

ino_t file_alloc();
/****************************************************************/
void file_ref(ino_t inum);
/****************************************************************/
void file_unref(ino_t inum);
/****************************************************************/
ret_t file_fetch(fs_t fs, void *key, void *dest, void **pdata, ino_t *pinum);
/****************************************************************/
si32_t fildes_alloc(si32_t min_fildes);
/****************************************************************/
void fildes_ref(si32_t fildes);
/****************************************************************/
void fildes_unref(si32_t fildes);
/****************************************************************/
ret_t fildes_check(si32_t fildes,
                   struct fildes **fd,
                   struct file **file,
                   bool_t usage,
                   fs_t fs);

#endif
