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

	/** Connection info. **/

	sock->state = TCP_CLOSED;
	sock->state_change_date = tics;
	sock->last_fin_ack_date = 0;
	sock->source_ip = 0;
	sock->dest_ip = 0;
	sock->source_port = 0;
	sock->dest_port = 0;
	/** FIXME Find a way to deal with sequence numbers wrapping. **/
	sock->seq_num = tics % 0x20000000;
	sock->ack_num = 0;

	/** Stats. **/

	sock->rtt = CLK_FREQ / 5;
	sock->vrtt = CLK_FREQ / 20;

	/** Buffers. **/

	tcp_reset(sock);
	sock->req_head = sock->req_tail = 0;
	sock->free_req = TCP_REQ_PER_SOCK;
}

/**
 * tcp_reset
 */

void tcp_reset(struct tcp_socket *sock)
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

		if((!sock->used && sock->state == TCP_CLOSED)
		|| (!sock->source_ip && sock->reuse_addr))
		{
			continue;
		}

		if(sock->source_port == port)
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
		&& (tmp_sock->state == TCP_LISTENING
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

void tcp_set_socket_state(struct tcp_socket *sock, sock_state_t state)
{
	if(sock->state == state)
	{
		sock->state_change_date = tics;
	}
	else
	{
		sock->state_change_date = tics - CLK_FREQ - 1;
	}

	sock->state = state;
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
	struct tcp_socket *sock;
	bool_t local_sync, remote_sync;
	ui32_t syn_ack;

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

	/** Connection opening stuff. **/

	if(tcp_socket_timeout(sock, CLK_FREQ))
	{
		if(sock->state == TCP_SYN_SENT)
		{
			tcp_send(sock, sock->seq_num - 1, 0, 0, TCP_SYN);
			tcp_set_socket_state(sock, TCP_SYN_SENT);
		}
		else if(sock->state == TCP_SYN_ACK_SENT)
		{
			tcp_send(sock, sock->seq_num - 1, 0, 0, TCP_SYN
			                                      | TCP_ACK);
			tcp_set_socket_state(sock, TCP_SYN_ACK_SENT);
		}
	}

	/** Try to actively close if there is nothing to transmit or the remote
	    host doesn't want to acknowledge data any longer. */

	if(sock->state == TCP_WAIT_ACTIVE_CLOSE
	&& (sock->free_buf == TCP_BUF_PER_SOCK
	 || sock->buf_tab[sock->buf_head].state == TCP_FAIL))
	{
		#ifdef DEBUG_TCP
		printk("trying to close socket %x\n", sock_id);
		#endif
		//tcp_send_fin(sock, 0);
		sock->seq_num++;
		tcp_set_socket_state(sock, TCP_FIN_WAIT1);
	}

	/** If the actively closing socket timed out... **/

	if(tcp_socket_timeout(sock, TCP_SOCK_DELAY))
	{
		if(sock->state == TCP_FIN_WAIT1
		|| sock->state == TCP_FIN_WAIT2
		|| sock->state == TCP_FIN_WAIT3
		|| sock->state == TCP_CLOSING2)
		{
			/** We failed actively closing the connection. **/

			tcp_set_socket_state(sock, TCP_TIME_WAIT);
		}
		else if(sock->state == TCP_TIME_WAIT)
		{
			/** We have been waiting for enough time, close the
			    socket. **/

			tcp_set_socket_state(sock, TCP_CLOSED);
		}
	}

	/** If our FIN wasn't taken into account... **/

	if(tics > (sock->last_fin_ack_date + CLK_FREQ)
	&& (sock->state == TCP_FIN_WAIT1
	 || sock->state == TCP_FIN_WAIT2
	 || sock->state == TCP_CLOSING2))
	{
		#ifdef DEBUG_TCP
		printk("resending fin-ack (socket %x)\n", sock_id);
		#endif
		tcp_send_fin(sock, 1);
	}

	/** If there is nothing to transmit, return. **/

	if(sock->free_buf == TCP_BUF_PER_SOCK)
	{
		if(sock->state == TCP_CLOSING1)
		{
			tcp_set_socket_state(sock, TCP_CLOSING2);
			sock->seq_num++;
			//tcp_send_fin(sock, 0);
		}
		
		return;
	}

	/** (Re)transmit as many packets as possible. **/

	buf_head = sock->buf_head;

	while(buf_head != sock->buf_tail && free_tx_desc)
	{
		buf = &sock->buf_tab[buf_head];

		if((tics <= buf->state_change_date + sock->rtt + 4 * sock->vrtt
		    && buf->state == TCP_WAITING_ACK))
		{
			goto next;
		}

		if(buf->state == TCP_FREE)
		{
			panic("unexpected free buffer");
		}

		if(buf->try_cnt > TCP_NET_RETRY)
		{
			tcp_set_buffer_state(sock, buf, TCP_FAIL);
		}

		if(tcp_send(sock, buf->seq_num, buf->data, buf->size, TCP_ACK)
		   != OK
		|| buf->state == TCP_FAIL)
		{
			#ifdef DEBUG_TCP
			if(buf->state == TCP_FAIL) { panic("tcp failure"); }
			#endif
			break;
		}

		tcp_set_buffer_state(sock, buf, TCP_WAITING_ACK);
		buf->try_cnt++;
	next:
		buf_head = (buf_head + 1) % TCP_BUF_PER_SOCK;
	}
}

/**
 * tcp_socket_timeout
 */

bool_t tcp_socket_timeout(struct tcp_socket *sock, clock_t timeout)
{
	return tics > sock->state_change_date + timeout;
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
	header->ack_num = (flags & TCP_ACK) ? htonl(sock->ack_num) : 0;
	header->res = 0;
	header->header_size = 5;
	header->flags = flags;
	header->wnd = htons(fifo_left(&sock->rx_fifo));
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
	ret_t ret = tcp_send(sock, sock->seq_num, 0, 0, TCP_ACK);

	#ifdef DEBUG_TCP
	if(ret != OK)
	{
		printk("socket %x failed sending ack\n", sock - tcp_sock_tab);
	}
	#endif

	return ret;
}

/**
 * tcp_send_fin
 */

ret_t tcp_send_fin(struct tcp_socket *sock, bool_t again)
{
	ui32_t seq_num = again ? (sock->seq_num - 1) : sock->seq_num;
	ret_t ret;

	if(!again)
	{
		sock->seq_num++;
	}

	ret = tcp_send(sock, seq_num, 0, 0, TCP_FIN | TCP_ACK);

	if(ret == OK)
	{
		sock->last_fin_ack_date = tics;
	}

	return ret;
}

/**
 * tcp_data_in
 */

ret_t tcp_data_in(struct tcp_socket *sock,
                 ui32_t seq_num,
                 void *data,
                 size_t size)
{
	if(sock->state < TCP_CONNECTED)
	{
		return -ENOTCONN;
	}

	if(seq_num == sock->ack_num)
	{
		if(sock->state == TCP_CONNECTED)
		{
			if(fifo_left(&sock->rx_fifo) < size)
			{
				return -ENOMEM;
			}

			if(fifo_write(&sock->rx_fifo, data, size, 0) != 0)
			{
				panic("critical error with fifo");
			}

			//printk("to read: %x\n", sock->rx_fifo.to_read);
		}

		sock->ack_num += size;

	}

	printk("acking data\n");
	tcp_send_ack(sock);

	return -EINVAL; // FIXME
}

/**
 * tcp_data_ack
 */

// Called when the remote host acknowledges data.

ret_t tcp_data_ack(struct tcp_socket *sock, ui32_t ack_num)
{
	struct tcp_buffer *buf = &sock->buf_tab[sock->buf_head];
	float rtt = (float)(tics - buf->state_change_date);

	while(sock->free_buf < TCP_BUF_PER_SOCK
	   && buf->state == TCP_WAITING_ACK
	   && (buf->seq_num + buf->size) <= ack_num)
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

	if(sock->free_buf * TCP_BUF_SIZE < size)
	{
		return -ENOMEM;
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
		buf->seq_num = sock->seq_num;
		buf->size = copy_size;
		buf->try_cnt = 0;

		sock->seq_num += copy_size;
		data += copy_size;
		size -= copy_size;
		sock->buf_tail = (sock->buf_tail + 1) % TCP_BUF_PER_SOCK;
	}

	return OK;
}
