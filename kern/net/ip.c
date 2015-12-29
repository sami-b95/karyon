/****************************************************************
 * ip.c                                                         *
 *                                                              *
 *    IP implementation.                                        *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <kernel/errno.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <net/endian.h>
#include <net/ether.h>
#include <net/tcp.h>

#define _IP_C_
#include "ip.h"

struct ip_buffer buf_tab[NR_IP_BUF] = {
	{
		.used = 0,
		.date = 0
	}
};

/**
 * ip_receive
 */

void ip_receive(struct ip_header *frag, size_t size)
{
	ui16_t checksum = frag->checksum;
	ui16_t frag_start, frag_end;
	ui32_t buf_id;
	ui32_t packet_delay;
	struct ip_buffer *buf = 0;
	struct ip_hole *hole, *prev_hole, *new_hole;
	ui16_t old_hole_end, next_hole;

	#ifdef DEBUG
	/*{
		size_t i;

		for(i = 0; i < size; i += 4)
		{
			if(!(i % 16))
			{
				printk("\n");
			}

			printk("%x ", *(ui32_t*)((ui8_t*)frag + i));
		}
	}*/
	#endif

	/** Check whether the fragment is for us. Check IP version. **/

	#ifdef IP_LOCAL_ADDR_ONLY
	if(frag->dest_ip != local_ip)
	{
		#ifdef DEBUG
		printk("dropping fragment for ip %x\n", frag->dest_ip);
		#endif
	}
	#endif

	if(frag->vers != 4)
	{
		#ifdef DEBUG
		printk("non-IPv4 fragment\n");
		#endif

		return;
	}

	/** Compute data related to the fragment, verify checksum **/

	frag_start = (ntohs(frag->pos_flags) & 0x1fff) * 8;
	frag_end = frag_start + ntohs(frag->total_size) - frag->ihl * 4 - 1;

	#ifdef DEBUG
	printk("***********************************************\n");
	printk("start: %x | end: %x\n", frag_start, frag_end);
	printk("source ip: %x | dest ip: %x\n",
	       frag->source_ip, frag->dest_ip);
	#endif

	if(frag_end >= IP_BUF_SIZE)
	{
		#ifdef DEBUG
		printk("ip buffer too small (%x, %x required at least)\n",
		       IP_BUF_SIZE, frag_end + 1);
		#endif

		return;
	}

	frag->checksum = 0;
	frag->checksum = ip_checksum((ui16_t*)frag, frag->ihl * 4);

	if(frag->checksum != checksum)
	{
		#ifdef DEBUG
		printk("received checksum: %x ; calculated checksum: %x\n",
		       checksum, frag->checksum);
		#endif

		return;
	}

	/** Check whether other fragments of the same fragment have been
	    received. If so, get the corresponding buffer. If not, get an
	    unused buffer. **/

	buf = 0;

	for(buf_id = 0; buf_id < NR_IP_BUF; buf_id++)
	{
		if(buf_tab[buf_id].used
		&& buf_tab[buf_id].id == frag->id
		&& buf_tab[buf_id].proto == frag->proto
		&& buf_tab[buf_id].source_ip == frag->source_ip
		&& buf_tab[buf_id].dest_ip == frag->dest_ip)
		{
			buf = &buf_tab[buf_id];

			#ifdef DEBUG
			printk("found buffer: %x\n", buf_id);
			#endif

			break;
		}

		/** TODO: Use appropriate TTL instead of IP_DELAY **/

		packet_delay = (tics - buf_tab[buf_id].date) / CLK_FREQ;

		if(!buf && (!buf_tab[buf_id].used || packet_delay > IP_DELAY))
		{
			buf = &buf_tab[buf_id];

			#ifdef DEBUG
			if(buf_tab[buf_id].used)
			{
				printk("dropping packet %x\n", buf - buf_tab);
			}

			printk("choosing buffer %x\n", buf - buf_tab);
			#endif

			buf->used = 0;
		}
	}

	/** Check for errors. **/

	if(!buf)
	{
		#ifdef DEBUG
		printk("no ip buffer left\n");
		#endif

		return;
	}

	/** Check whether we have just received the last fragment. It allows us
	    to determine the size of the packet. **/
	
	if(!(frag->pos_flags & IP_MF))
	{
		#ifdef DEBUG
		printk("received last fragment for packet %x\n",
		       buf - buf_tab);
		#endif

		buf->size = (size_t)frag_end + 1;
	}

	/** If we have other fragments, add current fragment to buffer. **/

	if(buf->used)
	{
		/**  We already have other fragments. **/

		#ifdef DEBUG
		printk("new fragment for packet %x\n", buf - buf_tab);
		#endif

		if(!buf->first_hole)
		{
			panic("buffer is used but first hole is null\n");
		}

		hole = buf->first_hole;

		/** prev_hole represents the previous valid hole (i.e not
		    deleted yet). **/

		prev_hole = 0;

		/** Deal with all the holes according to RFC815 algorithm. **/

		while(1)
		{
			/** Backup data from the deleted hole. **/

			old_hole_end = hole->end;
			next_hole = hole->next;
			printk("next_hole: %x\n", hole->next);

			/** If the fragment does not interact with the current
			    hole, there is nothing to do (except keep prev_hole
			    up to date). **/

			if(frag_start > hole->end || frag_end < hole->start)
			{
				prev_hole = hole;
				goto next;
			}

			/** Delete the current hole from the hole list. **/

			if(prev_hole)
			{
				prev_hole->next = next_hole;
			}
			else
			{
				buf->first_hole =
				 next_hole ? (struct ip_hole*)
				              &buf->data[next_hole]
				            : 0;
			}

			/** Add a new hole at the beginning of the old hole if
			    necessary. **/

			if(frag_start > hole->start)
			{
				/** If there is no room for the hole, an error
				    must have occured, abort the reception of
				    the current packet. **/

				if(frag_start - hole->start < 8)
				{
					#ifdef DEBUG
					printk("aborting %x (hole pb)\n",
					       buf - buf_tab);
					#endif

					buf->used = 0;

					return;
				}

				hole->end = frag_start - 1;

				if(prev_hole)
				{
					prev_hole->next = hole->start;
				}
				else
				{
					buf->first_hole = (struct ip_hole*)
					            &buf->data[hole->start];
				}

				prev_hole = hole;
			}

			/** Add a new hole at the end of the old hole if
			    necessary. **/

			if(frag_end < old_hole_end
			&& (!buf->size || old_hole_end < buf->size))
			{
				if(old_hole_end - frag_end < 8)
				{
					#ifdef DEBUG
					printk("aborting %x (hole pb 2)\n",
					       buf - buf_tab);
					#endif

					buf->used = 0;

					return;
				}

				new_hole = (struct ip_hole*)
				    &buf->data[frag_end + 1];
				new_hole->start = frag_end + 1;
				new_hole->end = old_hole_end;
				new_hole->next = next_hole;

				if(prev_hole)
				{
					prev_hole->next = frag_end + 1;
				}
				else
				{
					buf->first_hole = (struct ip_hole*)
					           &buf->data[frag_end + 1];
				}

				prev_hole = new_hole;
			}

			next:

			if(!next_hole)
			{
				break;
			}

			hole = (struct ip_hole*)&buf->data[next_hole];
		}
	}
	else
	{
		/** The current fragment is the only one we have received up to
		    now. **/

		#ifdef DEBUG
		printk("first received frag for packet %x\n", buf - buf_tab);
		#endif

		/** Initialize the buffer. **/

		buf->used = 1;
		buf->date = tics;
		buf->first_hole = 0;
		buf->id = frag->id;
		buf->proto = frag->proto;
		buf->source_ip = frag->source_ip;
		buf->dest_ip = frag->dest_ip;

		/** The buffer size may have been set above (if we have just
		    received the last fragment of the packet). **/

		if(!buf->size)
		{
			buf->size = 0;
		}

		/** Initialize holes if necessary. **/

		hole = new_hole = 0;

		if(frag_start)
		{
			/** Hole at the beginning. **/

			if(frag_start < 8)
			{
				#ifdef DEBUG
				printk("not enough room for hole\n");
				#endif

				buf->used = 0;

				return;
			}

			hole = (struct ip_hole*)buf->data;
			hole->start = 0;
			hole->end = frag_start - 1;
			hole->next = 0;
		}

		/** Don't make a hole at the end if we have the last
		    fragment. **/

		if(frag->pos_flags & IP_MF)
		{
			if(frag_end == 0xffff)
			{
				#ifdef DEBUG
				printk("pkt %x is too big\n", buf - buf_tab);
				#endif

				buf->used = 0;

				return;
			}

			/** Hole at the end. **/

			if(0xffff - frag_end < 8)
			{
				#ifdef DEBUG
				printk("not enough room for hole\n");
				#endif

				buf->used = 0;

				return;
			}

			new_hole = (struct ip_hole*)&buf->data[frag_end + 1];
			new_hole->start = frag_end + 1;
			new_hole->end = 0xffff;
			new_hole->next = 0;

			if(hole)
			{
				hole->next = frag_end + 1;
			}
		}

		/** Try to initialize buf->first_hole. It is null if the whole
		    packet has been received. **/

		buf->first_hole = hole ? hole : new_hole;
	}

	/** Copy the data of the fragment to the buffer. **/

	memcpy(buf->data + frag_start,
	       (void*)frag + frag->ihl * 4,
	       frag_end - frag_start + 1);

	/** Check whether reception is complete. If so, process the packet. **/

	if(!buf->first_hole)
	{
		#ifdef DEBUG
		printk("reception complete for packet %x\n", buf - buf_tab);
		printk("buf->data: %x\n", buf->data);
		#endif

		switch(buf->proto)
		{
			#ifdef DEBUG
			case IP_PROTO_ICMP:
				printk("icmp packet\n");
				break;
			#endif

			case IP_PROTO_TCP:
				#ifdef DEBUG
				printk("tcp packet\n");
				#endif
				tcp_receive((void*)buf->data,
				            buf->size,
				            buf->source_ip,
				            buf->dest_ip);
				break;

			default:
				#ifdef DEBUG
				printk("unrecognized proto %x\n", buf->proto);
				#endif
				break;
		}

		buf->used = 0;
	}
}

/**
 * ip_send
 */

ret_t ip_send(void *data,
              size_t size,
              ui8_t proto,
              ui32_t source_ip,
              ui32_t dest_ip,
              ui16_t flags)
{
	struct ip_header *header = data - 20;

	/** Check packet size. **/

	if(size > 1482)
	{
		#ifdef DEBUG
		printk("ip packet is too big (%x bytes)\n", size);
		#endif

		return -ENOMEM;
	}

	/** Prepare IP header. **/

	header->ihl = 5;
	header->vers = 4;
	header->service = 0;
	header->total_size = htons((ui16_t)size + 20);
	header->id = (ui16_t)tics;
	header->pos_flags = flags;
	header->ttl = IP_TTL;
	header->proto = proto;
	header->checksum = 0;
	header->source_ip = source_ip;
	header->dest_ip = dest_ip;
	header->checksum = 0;
	header->checksum = ip_checksum((ui16_t*)header, 20);

	/** Send the packet to the Ethernet layer. **/

	if((dest_ip & net_mask) == (local_ip & net_mask))
	{
		return ether_send((ui8_t*)header,
	        	          size + 20,
	        	          ETHER_PROTO_IPV4,
	        	          broadcast_hwaddr);
	}

	return ether_send((ui8_t*)header,
	        	          size + 20,
	        	          ETHER_PROTO_IPV4,
	        	          gw_hwaddr);
}

/**
 * ip_checksum
 */

ui16_t ip_checksum(ui16_t *data, size_t size)
{
	ui32_t accum = 0;

	while(size > 1)
	{
		accum += *data++;
		size -= 2;
	}

	if(size)
	{
		accum += *(ui8_t*)data;
	}

	accum = (accum >> 16) + (accum & 0xffff);
	accum = (accum >> 16) + accum;

	return (ui16_t)~accum;
};
