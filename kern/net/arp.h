#ifndef _ARP_H_
#define _ARP_H_

#include <kernel/types.h>

/** ARP header. **/

struct arp_header
{
	ui16_t net;
	ui16_t prot;
	ui8_t hwaddr_length;
	ui8_t laddr_length; // logical address length
	ui16_t op;
	ui8_t sender_hwaddr[6];
	ui32_t sender_ipaddr;
	ui8_t target_hwaddr[6];
	ui32_t target_ipaddr;
} __attribute__((packed));

/** Network type **/

#define ARP_NET_ETHER	0x0100

/** Protocol **/

#define ARP_PROT_IP	0x0008

/** Options **/

#define ARP_OP_REQ	0x0100
#define ARP_OP_REPLY	0x0200

/** Functions. **/

void arp_receive(struct arp_header *header);
/****************************************************************/
ret_t arp_request(ui32_t target_ipaddr);

#endif
