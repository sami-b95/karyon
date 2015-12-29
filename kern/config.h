#ifndef _CONFIG_H_
#define _CONFIG_H_

/** User-defined constants **/

#define DEF_TEXT_ATTR		0x07
#define RAM_SIZE		(96 * 1024 * 1024)
#define DISK_SIZE		104767488
#define SEC_OFF			0 // the sector the file system starts at
#define NR_PROC			64
#define NR_FILES		64
#define NR_FILDES		128
#define NR_FILDES_PER_PROC	32
#define NR_PIPES		16
#define ATA_CTL			0
#define ATA_SLAVE		0
#define NAME_MAX_LEN		255
#define PATH_MAX_LEN		1024
// Round buffer size
#define RBUF_SIZE		4096 
#define PIPE_ATOMIC_LIMIT	(RBUF_SIZE / 2)
// Number of disk blocks in the cache
#define NR_BLOCKS		4096
#define MAX_DIRTY_BLOCKS	2048
#define ATA_SEL_PIO
#define ATA_SEL_DELAY		10
//#define ATA_CACHE_FLUSH
#define TTY_DELAY		1
#define FIFO_DELAY		1
#define PAGING_ZERO
#define ENABLE_FPU
#define CLK_FREQ		100
//#define ENABLE_NETWORK
#define ENABLE_PIPES
// A received packet is not handled if there is no free transmit descriptor
#define RTL8139_TX_DESC_HACK
#define IP_BUF_SIZE	0x2000
#define NR_IP_BUF	4
// Max number of seconds a packet is considered valid.
#define IP_DELAY		9
#define IP_TTL			40
#define NR_TCP_SOCKS		16
#define TCP_BUF_SIZE		1460
#define TCP_BUF_PER_SOCK	8
#define TCP_REQ_PER_SOCK	8
#define TCP_MSS			1460
#define TCP_SSTHRESH		20000
// Number of retries before considering the packet lost in the network.
#define TCP_NET_RETRY		6
#define TCP_MAX_TRY_CNT		8
#define TCP_CLOSE_TRY_CNT	10
// Time a socket must wait after closing handshake (in clock tics).
#define TCP_SOCK_DELAY	60 * CLK_FREQ
#define TCP_DEF_TIMEOUT	2 * CLK_FREQ
#define TCP_KEEPALIVE

/** Debugging (the user should not touch these). **/

//#define DEBUG
//#define DEBUG_SCHEDULE
//#define DEBUG_ARP
//#define DEBUG_TCP
//#define DEBUG_RTL8139
//#define DEBUG_SOCKETS
//#define DEBUG_SYSCALLS
//#define DEBUG_EXT2
#define DEBUG_DELAY	50000
#define QEMU
#define PORT_0xE9_HACK
#define DEMAND_PAGING
#define PANIC_ON_EXC
#define USE_CACHE
/** Do not write back the current FAT sector to the disk if it was not
    modified. **/
#define USE_FAT32_LIMITED_WRITE
/** Remember the last free cluster in order to avoid looking up the whole FAT
    each time we want to allocate clusters. **/
#define USE_FAT32_FAST_LOOKUP
#define USE_SIGTTIO
#define EXTENDED_SIGNAL_HANDLING
//#define CACHE_SYNC_WHEN_FULL
#define SCREEN_NLCR
#define SIGNAL_USERMODE_ONLY
#define INTERRUPTIBLE_SYSCALLS
//#define TOUCHY_BAD_PF

#endif
