/****************************************************************
 * arp.c                                                        *
 *                                                              *
 *    Implementation of the ARP protocol.                       *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/printk.h>
#include <net/ether.h>
#include <net/ip.h>

#include "arp.h"

ui8_t arp_buf[ETHER_MIN_FRAME_SIZE];

/**
 * arp_receive
 */

void arp_receive(struct arp_header *header)
{
	struct arp_header *rep = (void*)arp_buf + 14;

	if(header->op == ARP_OP_REQ)
	{
		if(header->target_ipaddr == local_ip)
		{
			#ifdef DEBUG
			printk("sender hwaddr: ");
			printk("%x ", header->sender_hwaddr[0]);
			printk("%x ", header->sender_hwaddr[1]);
			printk("%x ", header->sender_hwaddr[2]);
			printk("%x ", header->sender_hwaddr[3]);
			printk("%x ", header->sender_hwaddr[4]);
			printk("%x ", header->sender_hwaddr[5]);
			printk("\n");
			#endif
			rep->net = ARP_NET_ETHER;
			rep->prot = ARP_PROT_IP;
			rep->hwaddr_length = 6;
			rep->laddr_length = 4;
			rep->op = ARP_OP_REPLY;
			memcpy(rep->sender_hwaddr, local_hwaddr, 6);
			rep->sender_ipaddr = local_ip;
			memcpy(rep->target_hwaddr, header->sender_hwaddr, 6);
			rep->target_ipaddr = header->sender_ipaddr;

			if(ether_send((ui8_t*)rep,
			              28,
			              ETHER_PROTO_ARP,
			              header->sender_hwaddr) != OK)
			{
				#ifdef DEBUG_ARP
				printk("failed replying to arp request\n");
				#endif
			}
		}
	}
	else if(header->op == ARP_OP_REPLY)
	{
		/** NOTE: We just handle the gateway for the moment. **/

		if(header->sender_ipaddr == gw_ip)
		{
			memcpy(gw_hwaddr, header->sender_hwaddr, 6);
			gw_hwaddr_set = 1;
		}
	}
	else
	{
		#ifdef DEBUG
		printk("unrecognized arp operation %x\n", header->op);
		#endif
	}
}

/**
 * arp_request
 */

ret_t arp_request(ui32_t target_ipaddr)
{
	struct arp_header *req = (void*)arp_buf + 14;

	req->net = ARP_NET_ETHER;
	req->prot = ARP_PROT_IP;
	req->hwaddr_length = 6;
	req->laddr_length = 4;
	req->op = ARP_OP_REQ;
	memcpy(req->sender_hwaddr, local_hwaddr, 6);
	req->sender_ipaddr = local_ip;
	memcpy(req->target_hwaddr, broadcast_hwaddr, 6);
	req->target_ipaddr = target_ipaddr;

	return ether_send((ui8_t*)req, 28, ETHER_PROTO_ARP, broadcast_hwaddr);
}
