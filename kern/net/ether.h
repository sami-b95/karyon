#ifndef _ETHER_H_
#define _ETHER_H_

#include <kernel/types.h>

/** Ethernet header structure. **/

struct ether_header
{
	ui8_t dest_addr[6];
	ui8_t source_addr[6];
	ui16_t proto;
} __attribute__((packed));

/** Protocols. **/

#define ETHER_PROTO_IPV4	0x0008
#define ETHER_PROTO_ARP		0x0608

/** Other. **/

#define ETHER_MIN_FRAME_SIZE	60

/** Global variables. **/

#ifdef _ETHER_C_
ui8_t local_hwaddr[6];
ui8_t gw_hwaddr[] = { 0x52, 0x1a, 0xd3, 0x5f, 0xf7, 0x69 };
ui8_t broadcast_hwaddr[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
bool_t gw_hwaddr_set = 0;
#else
extern ui8_t local_hwaddr[];
extern ui8_t gw_hwaddr[];
extern ui8_t broadcast_hwaddr[];
extern bool_t gw_hwaddr_set;
#endif

/** Functions. **/

void ether_receive(ui8_t *packet, size_t size);
/****************************************************************/
ret_t ether_send(ui8_t *packet, size_t size, ui16_t proto, ui8_t dest_addr[]);

#endif
