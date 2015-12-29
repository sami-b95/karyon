#ifndef _IP_H_
#define _IP_H_

#include <kernel/types.h>

/** IP header structure. **/

struct ip_header
{
	ui8_t ihl:4;
	ui8_t vers:4;
	ui8_t service; // usually set to 0
	ui16_t total_size;
	ui16_t id;
	ui16_t pos_flags;
	ui8_t ttl;
	ui8_t proto;
	ui16_t checksum;
	ui32_t source_ip;
	ui32_t dest_ip;
	ui8_t opt[3];
	ui8_t padding;
} __attribute__((packed));

/** Hole header. **/

	/**NOTE: this structure must be 8 byte long maximum (which is the
	         minimum length of the data of an IP packet). **/

struct ip_hole
{
	ui16_t start;
	ui16_t end;
	ui16_t next;
	ui16_t null;
} __attribute__((packed));

/** IP buffer. **/

struct ip_buffer
{
	bool_t used;
	ui32_t date;
	ui32_t id;
	ui8_t proto;
	ui32_t source_ip;
	ui32_t dest_ip;
	size_t size;
	struct ip_hole *first_hole;
	ui8_t checksum[12]; // reserved for calculating TCP checksum
	ui8_t data[IP_BUF_SIZE];
};

/** Protocols. **/

#define IP_PROTO_ICMP	0x01
#define IP_PROTO_TCP	0x06

/** Flags. **/

#define IP_MF	0x0020
#define IP_DF	0x0040

/** Others. **/

#define INADDR_ANY	0

/** Global variables. **/

#ifdef _IP_C_
ui32_t local_ip = 0x040010ac; // 172.16.0.4
ui32_t gw_ip = 0x030010ac; // 172.16.0.3
/*ui32_t local_ip = 0x4001a8c0; // 192.168.1.64
ui32_t gw_ip = 0xfe01a8c0; // 192.168.1.254*/
ui32_t net_mask = 0x00ffffff;
#else
extern ui32_t local_ip;
extern ui32_t gw_ip;
extern ui32_t net_mask;
#endif

/** Functions **/

void ip_receive(struct ip_header *frag, size_t size);
/****************************************************************/
ret_t ip_send(void *data,
              size_t size,
              ui8_t proto,
              ui32_t source_ip,
              ui32_t dest_ip,
              ui16_t flags);
/****************************************************************/
ui16_t ip_checksum(ui16_t *data, size_t size); // size in bytes

#endif
