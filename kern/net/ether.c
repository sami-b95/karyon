/****************************************************************
 * ether.c                                                      *
 *                                                              *
 *    Ethernet protocol implementation.                         *
 *                                                              *
 ****************************************************************/

#define _ETHER_C_
#include <config.h>
#include <kernel/errno.h>
#include <kernel/libc.h>
#include <kernel/printk.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/rtl8139.h>

#include "ether.h"

/**
 * ether_receive
 */

void ether_receive(ui8_t *packet, size_t size)
{
	struct ether_header *header = (struct ether_header*)packet;

	switch(header->proto)
	{
		case ETHER_PROTO_IPV4:
			#ifdef DEBUG
			printk(">> ip packet\n");
			#endif
			ip_receive((void*)packet + 14, size - 14);
			break;

		case ETHER_PROTO_ARP:
			#ifdef DEBUG
			printk(">> arp packet\n");
			#endif
			arp_receive((void*)packet + 14);
			break;

		default:
			#ifdef DEBUG
			printk(">> unrecognized proto %x\n", header->proto);
			#endif
			break;
	}
}

/**
 * ether_send
 */

ret_t ether_send(ui8_t *packet, size_t size, ui16_t proto, ui8_t dest_addr[])
{
	struct ether_header *header = (void*)packet - 14;

	memcpy(header->source_addr, local_hwaddr, 6);
	memcpy(header->dest_addr, dest_addr, 6);
	header->proto = proto;

	if(size + 14 < ETHER_MIN_FRAME_SIZE)
	{
		memset(packet + size, 0, ETHER_MIN_FRAME_SIZE - size - 14);
		size = ETHER_MIN_FRAME_SIZE - 14;
	}

	return rtl8139_send((void*)header, size + 14);
}
