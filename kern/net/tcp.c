/****************************************************************
 * tcp.c                                                        *
 *                                                              *
 *    Implementation of the TCP protocol.                       *
 *                                                              *
 ****************************************************************/

#define _TCP_C_

#include <config.h>
#include <fs/file.h>
#include <kernel/errno.h>
#include <kernel/int.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/libc.h>
#include <net/endian.h>
#include <net/ip.h>
#include <net/rtl8139.h>

#define _TCP_C_
#include "tcp.h"

/**
 * tcp_socket_init
 */

void tcp_socket_init(struct tcp_socket *sock)
{
	sock->used = 1;
	sock->reuse_addr = 0;

	/** State/timeouts. **/

	sock->state = TCP_CLOSED;
	sock->timeout_set_tics = tics;
	sock->timeout_tics = tics;
	sock->try_cnt = 0;

	/** Connection opening/closing. **/

	sock->active_open = 0;
	sock->close_req = 0;
	sock->reset = 0;

	/** Local/remote host info. **/

	sock->source_ip = 0;
	sock->dest_ip = 0;
	sock->source_port = 0;
	sock->dest_port = 0;

	/** Useful sequence numbers. **/

	/** FIXME Find a way to deal with sequence numbers wrapping. **/
	sock->iss = tics % 0x20000000;
	sock->fss = 0;
	sock->snd_una = 0;
	sock->snd_nxt = 0;
	sock->snd_wl1 = 0;
	sock->snd_wl2 = 0;
	sock->snd_wnd = 0;
	sock->irs = 0;
	sock->rcv_nxt = 0;
	sock->rcv_wnd = 0;

	/** Stats. **/

	sock->rtt = CLK_FREQ / 5;
	sock->vrtt = CLK_FREQ / 20;
	sock->buffer_timeout = sock->rtt + 4 * sock->vrtt;

	/** Buffers. **/

	tcp_buffers_reset(sock);
	sock->req_head = sock->req_tail = 0;
	sock->free_req = TCP_REQ_PER_SOCK;
}

/**
 * tcp_buffers_reset
 */

void tcp_buffers_reset(struct tcp_socket *sock)
{
	ui32_t i;

	fifo_flush(&sock->rx_fifo);

	for(i = 0; i < TCP_BUF_PER_SOCK; i++)
	{
		sock->buf_tab[i].state = TCP_FREE;
	}

	sock->buf_head = sock->buf_tail = 0;
	sock->free_buf = TCP_BUF_PER_SOCK;
}

/**
 * tcp_socket_alloc
 */

si32_t tcp_socket_alloc()
{
	si32_t sock_id;

	for(sock_id = 0; sock_id < NR_TCP_SOCKS; sock_id++)
	{
		if(tcp_sock_tab[sock_id].state == TCP_CLOSED
		&& !tcp_sock_tab[sock_id].used)
		{
			tcp_socket_init(&tcp_sock_tab[sock_id]);

			return sock_id;
		}
	}

	return -1;
}

/**
 * tcp_socket_free
 */

void tcp_socket_free(si32_t sock_id)
{
	if(!tcp_sock_tab[sock_id].used)
	{
		panic("socket %x already freed or unused", sock_id);
	}

	tcp_sock_tab[sock_id].used = 0;
}

/**
 * tcp_port_is_used
 */

bool_t tcp_port_is_used(ui16_t port)
{
	si32_t sock_id;
	struct tcp_socket *sock;

	for(sock_id = 0; sock_id < NR_TCP_SOCKS; sock_id++)
	{
		sock = &tcp_sock_tab[sock_id];

		if((sock->used || sock->state != TCP_CLOSED)
		&& (sock->source_ip || !sock->reuse_addr)
		&& sock->source_port == port)
		{
			return 1;
		}
	}

	return 0;
}

/**
 * tcp_port_alloc
 */

ui16_t tcp_port_alloc()
{
	ui16_t port;

	for(port = 1024; port; port++)
	{
		if(!tcp_port_is_used(port))
		{
			return port;
		}
	}

	return 0;
}

/**
 * tcp_find_socket
 */

struct tcp_socket *tcp_find_socket(ui32_t source_ip,
                                   ui16_t source_port,
                                   ui16_t dest_port)
{
	si32_t sock_id;
	struct tcp_socket *sock = 0, *tmp_sock;

	for(sock_id = 0; sock_id < NR_TCP_SOCKS; sock_id++)
	{
		tmp_sock = &tcp_sock_tab[sock_id];

		if((tmp_sock->source_port == dest_port)
		&& (tmp_sock->state == TCP_LISTEN
		    ||  (tmp_sock->dest_ip == source_ip
		      && tmp_sock->dest_port == source_port))
		&& (tmp_sock->used || tmp_sock->state != TCP_CLOSED)
		&& (!sock || !sock->source_ip))
		{
			sock = &tcp_sock_tab[sock_id];
		}
	}

	return sock;
}

/**
 * tcp_set_socket_state
 */

void tcp_set_socket_state(struct tcp_socket *sock,
                          sock_state_t state,
                          clock_t timeout)
{
	if(state != sock->state)
	{
		sock->try_cnt = 0;
	}

	sock->state = state;
	sock->timeout_set_tics = tics;
	sock->timeout_tics = tics + timeout;

	if(state == TCP_SYN_SENT)
	{
		sock->active_open = 1;
	}
}

/**
 * tcp_set_buffer_state
 */

void tcp_set_buffer_state(struct tcp_socket *sock,
                          struct tcp_buffer *buf,
                          buf_state_t state)
{
	if(state == TCP_FREE)
	{
		if(buf->state == TCP_FREE)
		{
			panic("free buffer freed again");
		}

		sock->free_buf++;
	}

	if(state != TCP_FREE && buf->state == TCP_FREE)
	{
		sock->free_buf--;
	}

	buf->state = state;
	buf->state_change_date = tics;
}

/**
 * tcp_request_push
 */

ret_t tcp_request_push(struct tcp_socket *sock,
                       ui32_t source_ip,
                       ui32_t dest_ip,
                       ui16_t source_port,
                       ui16_t dest_port,
                       ui32_t irs)
{
	struct tcp_request *req = &sock->req_tab[sock->req_tail];

	#ifdef DEBUG_TCP
	printk("request from %x:%x for socket %x (free req: %x)\n",
	       source_ip, source_port, sock - tcp_sock_tab, sock->free_req);
	#endif

	if(!sock->free_req)
	{
		return -ENOMEM;
	}

	req->source_ip = source_ip;
	req->dest_ip = dest_ip;
	req->source_port = source_port;
	req->dest_port = dest_port;
	req->irs = irs;

	sock->req_tail = (sock->req_tail + 1) % TCP_REQ_PER_SOCK;
	sock->free_req--;

	return OK;
}

/**
 * tcp_request_pop
 */

struct tcp_request *tcp_request_pop(struct tcp_socket *sock)
{
	struct tcp_request *req = &sock->req_tab[sock->req_head];

	if(sock->free_req == TCP_REQ_PER_SOCK)
	{
		return 0;
	}

	sock->req_head = (sock->req_head + 1) % TCP_REQ_PER_SOCK;
	sock->free_req++;

	return req;
}

/**
 * tcp_receive
 */

void tcp_receive(struct tcp_header *packet,
                 size_t size,
                 ui32_t source_ip,
                 ui32_t dest_ip)
{
	ui16_t checksum = packet->checksum;
	ui32_t seq_num = ntohl(packet->seq_num);
	ui32_t ack_num = ntohl(packet->ack_num);
	ui16_t source_port = ntohs(packet->source_port);
	ui16_t dest_port = ntohs(packet->dest_port);
	size_t data_size = size - 4 * packet->header_size;
	void *data = (void*)packet + 4 * packet->header_size;
	struct tcp_socket *sock;

	#ifdef DEBUG_TCP
	printk("\n******************* TCP header dump *******************\n");
	printk("* source port: %x | dest port: %x\n", source_port, dest_port);
	printk("* seq num: %x | ack num: %x\n", seq_num, ack_num);
	printk("* flags: %x | data size: %x\n", packet->flags, data_size);
	printk("*******************************************************\n\n");
	#endif

	/** Verify checksum **/

	packet->checksum = 0;
	packet->checksum = tcp_checksum(packet, size, source_ip, dest_ip);

	if(packet->checksum != checksum)
	{
		#ifdef DEBUG_TCP
		printk("wrong tcp checksum\n");
		#endif
		return;
	}

	/** Find which socket the segment is targetted to. **/

	sock = tcp_find_socket(source_ip, source_port, dest_port);

	if(!sock)
	{
		#ifdef DEBUG_TCP
		printk("tcp segment dropped: unable to find socket\n");
		#endif
		return;
	}

	/** Packet processing here. **/

	if(sock->state == TCP_CLOSED)
	{
		if(packet->flags & TCP_ACK)
		{
			tcp_send(sock, ack_num, 0, 0, 0, TCP_RST);
		}
		else
		{
			tcp_send(sock,
			         0,
			         seq_num + data_size,
			         0,
			         0,
			         TCP_RST | TCP_ACK);
		}
	}
	else if(sock->state == TCP_LISTEN)
	{
		if(packet->flags & TCP_RST)
		{
			/** This is maybe a duplicate of a RST sent to abort
			    the connection instead of acking our SYN-ACK. **/
		}
		else if(packet->flags & TCP_ACK)
		{
			tcp_send(sock, ack_num, 0, 0, 0, TCP_RST);
		}
		else if(packet->flags & TCP_SYN)
		{ 
			if(tcp_request_push(sock,
			                    source_ip,
			                    dest_ip,
			                    source_port,
			                    dest_port,
			                    seq_num) != OK)
			{
				#ifdef DEBUG_TCP
				printk("could not push connection request\n");
				#endif
			}
		}
	}
	else if(sock->state == TCP_SYN_SENT)
	{
		if(packet->flags & TCP_ACK)
		{
			/** The packet has an acknowledgement number. **/

			if(ack_num == sock->iss + 1)
			{
				/** Our SYN is acknowledged. **/

				if(packet->flags & TCP_RST)
				{
					tcp_set_socket_state(sock,
					               TCP_CLOSED,
					                        0);
					sock->reset = 1;
					tcp_buffers_reset(sock);
				}
				else if(packet->flags & TCP_SYN)
				{
					sock->irs = seq_num;
					sock->rcv_nxt = seq_num + 1;
					sock->rcv_wnd = RBUF_SIZE;
					sock->snd_una = ack_num;
					tcp_set_socket_state(sock,
					                     TCP_ESTABLISHED,
					                     0);
					tcp_send_ack(sock);
				}
			}
			else
			{
				/** Invalid acknowledgement number. **/

				tcp_send(sock,
				         ack_num,
				         0,
				         0,
				         0,
				         TCP_RST);
			}
		}
		else if((packet->flags & TCP_SYN)
		    && !(packet->flags & TCP_RST))
		{
			sock->irs = seq_num;
			sock->rcv_nxt = seq_num + 1;
			sock->rcv_wnd = RBUF_SIZE;
			tcp_set_socket_state(sock, TCP_SYN_RECEIVED, 0);
		}
	}
	else
	{
		/** Check sequence number. **/

		if(seg_inter_win(seq_num,
		                 seq_num + data_size - 1,
		                 sock->rcv_nxt,
		                 sock->rcv_nxt + sock->rcv_wnd - 1))
		{
			if(packet->flags & TCP_RST)
			{
				if(sock->state == TCP_SYN_RECEIVED
				&& !sock->active_open)
				{
					tcp_set_socket_state(sock,
					                     TCP_LISTEN,
					                     0);
				}
				else
				{
					tcp_set_socket_state(sock,
					                     TCP_CLOSED,
					                     0);
				}

				sock->reset = 1;
				tcp_buffers_reset(sock);
			}
			else if(packet->flags & TCP_SYN)
			{
				if(packet->flags & TCP_ACK)
				{
					tcp_send(sock,
					         ack_num,
					         0,
					         0,
					         0,
					         TCP_RST);
				}
				else
				{
					tcp_send(sock,
						 0,
						 seq_num + data_size,
						 0,
						 0,
						 TCP_RST | TCP_ACK);
				}
			}
			else if(packet->flags & TCP_ACK)
			{
				/** Process acknowledgement. **/

				if(sock->state == TCP_ESTABLISHED
				|| sock->state == TCP_CLOSE_WAIT)
				{
					tcp_data_ack(sock, ack_num);
				}
				else if(ack_num == sock->snd_nxt)
				{
					if(sock->state == TCP_SYN_RECEIVED)
					{
						tcp_set_socket_state(sock,
						          TCP_ESTABLISHED,
						                        0);
					}
					else if(sock->state == TCP_FIN_WAIT1)
					{
						tcp_set_socket_state(sock,
						            TCP_FIN_WAIT2,
						                        0);
					}
					else if(sock->state == TCP_CLOSING)
					{
						tcp_set_socket_state(sock,
						            TCP_TIME_WAIT,
						           TCP_SOCK_DELAY);
					}
					else if(sock->state == TCP_LAST_ACK)
					{
						tcp_set_socket_state(sock,
						               TCP_CLOSED,
						                        0);
					}
				}

				/** Process segment data. **/

				if(sock->state == TCP_ESTABLISHED
				|| sock->state == TCP_FIN_WAIT1
				|| sock->state == TCP_FIN_WAIT2)
				{
					tcp_data_in(sock,
					            seq_num,
					            data,
					            data_size);
				}

				/** Process FIN. **/

				if(seq_num + data_size == sock->rcv_nxt
				&& (packet->flags & TCP_FIN))
				{
					sock->rcv_nxt++;

					if(sock->state == TCP_ESTABLISHED)
					{
						tcp_set_socket_state(sock,
						           TCP_CLOSE_WAIT,
						                        0);
					}
					else if(sock->state == TCP_FIN_WAIT1)
					{
						tcp_set_socket_state(sock,
						              TCP_CLOSING,
						          TCP_DEF_TIMEOUT);
					}
					else if(sock->state == TCP_FIN_WAIT2)
					{
						tcp_set_socket_state(sock,
						            TCP_TIME_WAIT,
						           TCP_SOCK_DELAY);
					}

					tcp_send_ack(sock);
				}

				/** Update send window. **/

				if(mod2pow32_compare(seq_num,
				                     sock->snd_wl1) > 0
				|| (!mod2pow32_compare(seq_num,
				                       sock->snd_wl1)
				 && mod2pow32_compare(ack_num,
					              sock->snd_wl2) > 0))
				{
					sock->snd_wl1 = seq_num;
					sock->snd_wl2 = ack_num;
					sock->snd_wnd = 0;
				}
			}
		}
		else
		{
			tcp_send_ack(sock);
		}
	}
}

/**
 * tcp_callback
 */

void tcp_callback()
{
	static si32_t prev_sock_id = NR_TCP_SOCKS - 1;
	si32_t sock_id;
	struct tcp_socket *sock;
	off_t buf_head;
	struct tcp_buffer *buf;

	/** Get the first used socket following the one we dealt with in
	    the previous call to this function. **/

	sock_id = (prev_sock_id + 1) % NR_TCP_SOCKS;

	while(sock_id != prev_sock_id)
	{
		if(tcp_sock_tab[sock_id].state != TCP_CLOSED
		|| tcp_sock_tab[sock_id].used)
		{
			break;
		}

		sock_id = (sock_id + 1) % NR_TCP_SOCKS;
	}

	prev_sock_id = sock_id;
	sock = &tcp_sock_tab[sock_id];

	if(!sock->used && sock->state == TCP_CLOSED)
	{
		return;
	}

	/** Connection opening and closing stuff. **/

	if(tcp_socket_timeout(sock))
	{
		if(sock->state == TCP_SYN_SENT)
		{
			if(sock->try_cnt > TCP_MAX_TRY_CNT)
			{
				tcp_set_socket_state(sock,
				               TCP_CLOSED,
				                        0);
			}
			else if(tcp_send(sock,
			                 sock->iss,
			                 0,
			                 0,
			                 0,
			                 TCP_SYN) == OK)
			{
				#ifdef DEBUG_TCP
				printk("syn timeout\n");
				#endif

				tcp_set_socket_state(sock,
				                     TCP_SYN_SENT,
				                     TCP_DEF_TIMEOUT);
				sock->try_cnt++;
			}
		}
		else if(sock->state == TCP_SYN_RECEIVED)
		{
			if(sock->iss + 1 != sock->snd_nxt)
			{
				panic("corrupt snd info (connection init)");
			}
			else if(sock->irs + 1 != sock->rcv_nxt)
			{
				panic("corrupt rcv info (connection init)");
			}

			if(sock->try_cnt > TCP_MAX_TRY_CNT)
			{
				tcp_set_socket_state(sock,
				               TCP_CLOSED,
				                        0);
			}
			else if(tcp_send(sock,
			                 sock->iss,
			                 sock->rcv_nxt,
			                 0,
			                 0,
			                 TCP_SYN | TCP_ACK) == OK)
			{
				tcp_set_socket_state(sock,
				                     TCP_SYN_RECEIVED,
				                     TCP_DEF_TIMEOUT);
				sock->try_cnt++;
			}
		}
		else if(sock->state == TCP_FIN_WAIT1
		     || sock->state == TCP_CLOSING
		     || sock->state == TCP_LAST_ACK)
		{
			if(sock->try_cnt > TCP_MAX_TRY_CNT)
			{
				tcp_set_socket_state(sock,
				               TCP_CLOSED,
				                        0);
			}
			else if(tcp_send(sock,
			                 sock->fss,
			                 sock->rcv_nxt,
			                 0,
			                 0,
			                 TCP_FIN | TCP_ACK) == OK)
			{
				tcp_set_socket_state(sock,
				                     sock->state,
				                     TCP_DEF_TIMEOUT);
				sock->try_cnt++;
			}
		}
	}

	/** If all the buffers were sent and the user requested a close, update
	    the socket state. **/

	if(sock->free_buf == TCP_BUF_PER_SOCK && sock->close_req)
	{
		if(sock->state == TCP_ESTABLISHED)
		{
			tcp_set_socket_state(sock, TCP_FIN_WAIT1, 0);
			sock->fss = sock->snd_nxt;
			sock->snd_nxt++;
		}
		else if(sock->state == TCP_CLOSE_WAIT)
		{
			tcp_set_socket_state(sock, TCP_LAST_ACK, 0);
			sock->fss = sock->snd_nxt;
			sock->snd_nxt++;
		}
	}

	/** Check whether we can send. **/

	if(sock->state != TCP_ESTABLISHED
	&& sock->state != TCP_CLOSE_WAIT)
	{
		return;
	}

	/** (Re)transmit as many packets as possible. **/

	buf_head = sock->buf_head;

	while(buf_head != sock->buf_tail && free_tx_desc)
	{
		buf = &sock->buf_tab[buf_head];

		if((buf->state != TCP_WAITING_ACK
		    || !tcp_buffer_timeout(sock, buf))
		&&  buf->state != TCP_WAITING_SEND)
		{
			goto next;
		}

		if(buf->state == TCP_FREE)
		{
			panic("unexpected free buffer");
		}

		if(buf->try_cnt > TCP_MAX_TRY_CNT)
		{
			tcp_set_socket_state(sock, TCP_CLOSED, 0);
			tcp_buffers_reset(sock);
			break;
		}

		if(tcp_send(sock,
		            buf->seq_num,
		            sock->rcv_nxt,
		            buf->data,
		            buf->size,
		            TCP_ACK) == OK)
		{
			tcp_set_buffer_state(sock, buf, TCP_WAITING_ACK);
			buf->try_cnt++;
		}
		else
		{
			break;
		}

	next:
		buf_head = (buf_head + 1) % TCP_BUF_PER_SOCK;
	}
}

/**
 * tcp_socket_timeout
 */

bool_t tcp_socket_timeout(struct tcp_socket *sock)
{
	/** WARNING: Don't add -1 to sock->timeout_tics, just in case
	    sock->timeout_tics = sock->timeout_set_tics. **/

	return !byte_in_win(tics, sock->timeout_set_tics, sock->timeout_tics);
}

/**
 * tcp_buffer_timeout
 */

bool_t tcp_buffer_timeout(struct tcp_socket *sock, struct tcp_buffer *buf)
{
	/** WARNING: Same as above. **/

	return !byte_in_win(tics,
	                    buf->state_change_date,
	                    buf->state_change_date + sock->buffer_timeout);
}

/**
 * tcp_checksum
 */

ui16_t tcp_checksum(struct tcp_header *packet,
                    size_t size,
                    ui32_t source_ip,
                    ui32_t dest_ip)
{
	struct tcp_pseudo_header *pseudo_hdr = (void*)packet - 12;

	pseudo_hdr->source_ip = source_ip;
	pseudo_hdr->dest_ip = dest_ip;
	pseudo_hdr->mbz = 0;
	pseudo_hdr->type = 6;
	pseudo_hdr->size = htons(size);

	packet->checksum = 0;

	return ip_checksum((ui16_t*)pseudo_hdr, size + 12);
}

/**
 * tcp_send
 */

ret_t tcp_send(struct tcp_socket *sock,
               ui32_t seq_num,
               ui32_t ack_num,
               void *data,
               size_t size,
               ui32_t flags)
{
	static ui8_t buf[TCP_BUF_SIZE + 56];
	struct tcp_header *header = (void*)buf + 36;

	if(size > TCP_BUF_SIZE)
	{
		return -EINVAL;
	}

	/** Configure the header. **/ 

	header->source_port = htons(sock->source_port);
	header->dest_port = htons(sock->dest_port);
	header->seq_num = htonl(seq_num);
	header->ack_num = htonl(ack_num);
	header->res = 0;
	header->header_size = 5;
	header->flags = flags;
	header->wnd = (sock->rcv_wnd < 0.30 * RBUF_SIZE
	            ? 0
	            : htons(sock->rcv_wnd));
	header->checksum = 0;
	header->urg_ptr = 0;

	/** If the packet contains data, copy them. **/

	if(data)
	{
		memcpy((void*)header + 20, data, size);
	}

	/** Calculate checksum. **/

	header->checksum = tcp_checksum(header,
	                                size + 20,
	                                sock->source_ip,
	                                sock->dest_ip);

	/** Pass the packet to the IP layer. **/

	return ip_send((void*)header,
                       size + 20,
	               IP_PROTO_TCP,
                       sock->source_ip,
                       sock->dest_ip,
                       0);
}

/**
 * tcp_send_ack
 */

ret_t tcp_send_ack(struct tcp_socket *sock)
{
	ret_t ret = tcp_send(sock, sock->snd_nxt, sock->rcv_nxt, 0, 0, TCP_ACK);

	#ifdef DEBUG_TCP
	if(ret != OK)
	{
		printk("socket %x failed sending ack\n", sock - tcp_sock_tab);
	}
	#endif

	return ret;
}

/**
 * tcp_data_in
 */

// The sequence number is supposed to be valid.

void tcp_data_in(struct tcp_socket *sock,
                 ui32_t seq_num,
                 void *data,
                 size_t size)
{
	size_t read = 0, written = 0;

	/** If no data, return. **/

	if(!size)
	{
		return;
	}

	/** Send an ACK. **/

	tcp_send_ack(sock);

	/** If a block of data between this one is missing, return. **/

	if(byte_in_win(seq_num,
	               sock->rcv_nxt,
	               sock->rcv_nxt + sock->rcv_wnd - 1)
	&& seq_num != sock->rcv_nxt)
	{
		return;
	}

	/** Escape already received data. **/

	while(seq_num != sock->rcv_nxt)
	{
		seq_num++;
		data++;
		read++;
	}

	if(read >= size)
	{
		panic("invalid sequence number or invalid receive window");
	}

	/** Write new data to received FIFO. **/

	while(seq_num != sock->rcv_nxt + sock->rcv_wnd && read < size)
	{
		if(fifo_write_byte(&sock->rx_fifo, *(ui8_t*)data, 0) != OK)
		{
			break;
		}

		seq_num++;
		data++;
		read++;
		written++;
	}

	sock->rcv_nxt += written;
	sock->rcv_wnd -= written;

	/*if(written)
	{
		tcp_send_ack(sock);
	}*/
}

/**
 * tcp_data_ack
 */

// Called when the remote host acknowledges data.

ret_t tcp_data_ack(struct tcp_socket *sock, ui32_t ack_num)
{
	struct tcp_buffer *buf = &sock->buf_tab[sock->buf_head];
	float rtt = (float)(tics - buf->state_change_date);

	if(sock->snd_una == sock->snd_nxt
	|| ack_num == sock->snd_una)
	{
		/** Nothing new to acknowledge. **/

		return OK;
	}

	if(!byte_in_win(ack_num - 1, sock->snd_una, sock->snd_nxt - 1))
	{
		/** At this point, we know that the remote host must have
		    acknowledged something new. Thus, if ack_num - 1 is not in
		    the send window, the ACK is considered invalid. **/

		return tcp_send_ack(sock);
	}

	/** Free as many buffers as possible. **/

	while(sock->free_buf < TCP_BUF_PER_SOCK
	   && buf->state == TCP_WAITING_ACK
	   && seg_in_win(buf->seq_num,
	                 buf->seq_num + buf->size - 1,
	                 sock->snd_una,
	                 ack_num - 1))
	{
		#ifdef DEBUG_TCP
		printk("buffer %x acked (seq: %x)\n",
		       sock->buf_head, buf->seq_num);
		#endif
		sock->rtt = (clock_t)(0.875f * (float)sock->rtt
		                    + 0.125f * rtt);
		sock->vrtt = (clock_t)(0.25f * abs(rtt - (float)sock->rtt)
		                     + 0.75f * sock->vrtt);
		tcp_set_buffer_state(sock, buf, TCP_FREE);
		sock->buf_head = (sock->buf_head + 1) % TCP_BUF_PER_SOCK;
		buf = &sock->buf_tab[sock->buf_head];
	}

	/** Update send state. **/

	sock->snd_una = ack_num;

	/** Update stats. **/

	sock->buffer_timeout = sock->rtt + 4 * sock->vrtt;

	#ifdef DEBUG_TCP
	printk("new rtt: %x\n", sock->rtt);

	if(sock->free_buf < TCP_BUF_PER_SOCK && buf->seq_num != ack_num)
	{
		panic("seq/ack numbers are unconsistent");
	}
	#endif

	return OK;
}

/**
 * tcp_data_out
 */

ret_t tcp_data_out(struct tcp_socket *sock, void *data, size_t size)
{
	size_t copy_size;
	struct tcp_buffer *buf = &sock->buf_tab[sock->buf_tail];

	if(sock->state != TCP_ESTABLISHED
	&& sock->state != TCP_CLOSE_WAIT)
	{
		return -ENOTCONN;
	}
	if(sock->close_req)
	{
		return -EPIPE;
	}
	else if(sock->free_buf * TCP_BUF_SIZE < size)
	{
		return -ENOBUFS;
	}

	while(size)
	{
		#ifdef DEBUG_TCP
		printk("allocating buffer %x\n", sock->buf_tail);
		#endif
		copy_size = (size > TCP_BUF_SIZE) ? TCP_BUF_SIZE : size;
		buf = &sock->buf_tab[sock->buf_tail];

		/** tcp_set_buffer_state() should decrease sock->free_buf. **/

		memcpy(buf->data, data, copy_size);
		tcp_set_buffer_state(sock, buf, TCP_WAITING_SEND);
		buf->seq_num = sock->snd_nxt;
		buf->size = copy_size;
		buf->try_cnt = 0;

		sock->snd_nxt += copy_size;
		data += copy_size;
		size -= copy_size;
		sock->buf_tail = (sock->buf_tail + 1) % TCP_BUF_PER_SOCK;
	}

	return OK;
}

/**
 * tcp_data_push
 */

size_t tcp_data_push(struct tcp_socket *sock, void *buf, size_t max_size)
{
	size_t ret = fifo_read(&sock->rx_fifo, buf, max_size, 0);

	sock->rcv_wnd += ret;

	return ret;	
}
