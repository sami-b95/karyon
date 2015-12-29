#ifndef _TCP_H_
#define _TCP_H_

#include <config.h>
#include <fs/fifo.h>
#include <kernel/types.h>

/** TCP header. **/

struct tcp_header
{
	ui16_t source_port;
	ui16_t dest_port;
	ui32_t seq_num;
	ui32_t ack_num;
	ui8_t res:4;
	ui8_t header_size:4;
	ui8_t flags;
	ui16_t wnd;
	ui16_t checksum;
	ui16_t urg_ptr;
	ui32_t opt_padding;
} __attribute__((packed));

/** Pseudo TCP header. **/

struct tcp_pseudo_header
{
	ui32_t source_ip;
	ui32_t dest_ip;
	ui8_t mbz;
	ui8_t type;
	ui16_t size;
} __attribute__((packed));

/** TCP connection request (for passive opening). **/

struct tcp_request
{
	ui32_t source_ip;
	ui32_t dest_ip;
	ui32_t source_port;
	ui32_t dest_port;
	ui32_t irs;
};

/** TCP buffer. **/

typedef enum {
	TCP_FREE = 1, // DEBUG
	TCP_WAITING_SEND = 2,
	TCP_WAITING_ACK = 3
} buf_state_t;

struct tcp_buffer
{
	buf_state_t state;
	ui32_t state_change_date;
	ui32_t seq_num;
	size_t size;
	count_t try_cnt;
	ui8_t data[TCP_BUF_SIZE + 56]; // 56: pseudo-header + TCP + IP + Eth
};

/** TCP socket. NOTE: the order matters. Don't change it. **/

typedef enum {
	TCP_CLOSED = 1,
	TCP_LISTEN = 2,
	TCP_SYN_SENT = 3,
	TCP_SYN_RECEIVED = 4,
	TCP_ESTABLISHED = 5,
	TCP_CLOSE_WAIT = 6,
	TCP_FIN_WAIT1 = 7,
	TCP_FIN_WAIT2 = 8,
	TCP_CLOSING = 9,
	TCP_LAST_ACK = 10,
	TCP_TIME_WAIT = 11
} sock_state_t;

struct tcp_socket
{
	bool_t used;
	bool_t reuse_addr;

	/** State/timeouts. **/

	sock_state_t state;
	clock_t timeout_set_tics;
	clock_t timeout_tics;
	count_t try_cnt;

	/** Connection info. **/

	bool_t active_open;
	bool_t close_req; // local user made a close request
	bool_t reset;

	/** Local/remote host info. **/

	ui32_t source_ip;
	ui32_t dest_ip;
	ui16_t source_port;
	ui16_t dest_port;

	/** Useful sequence numbers. **/

	ui32_t iss;
	ui32_t fss;
	ui32_t snd_una;
	ui32_t snd_nxt;
	ui32_t snd_wl1;
	ui32_t snd_wl2;
	ui32_t snd_wnd;
	ui32_t irs;
	ui32_t rcv_nxt;
	ui32_t rcv_wnd;

	/** Stats. **/

	clock_t rtt;
	clock_t vrtt;
	clock_t buffer_timeout;

	/** Buffers. **/

	struct fifo rx_fifo;
	struct tcp_buffer buf_tab[TCP_BUF_PER_SOCK];
	off_t buf_head, buf_tail;
	count_t free_buf;
	struct tcp_request req_tab[TCP_REQ_PER_SOCK];
	off_t req_head, req_tail;
	count_t free_req;
};

/** Flags. **/

#define TCP_FIN	0x01
#define TCP_SYN	0x02
#define TCP_RST	0x04
#define TCP_PSH	0x08
#define TCP_ACK	0x10
#define TCP_URG	0x20
#define TCP_ECN	0x40
#define TCP_CWR	0x80

/** Socket options. **/

#define SO_REUSEADDR	2

/** Global variables. **/
#ifdef _TCP_C_
struct tcp_socket tcp_sock_tab[NR_TCP_SOCKS] = {
	[0 ... (NR_TCP_SOCKS - 1)] = {
		.used = 0,
		.state = TCP_CLOSED
	}
};
#else
extern struct tcp_socket tcp_sock_tab[];
#endif

/** Functions. **/

	// NOTE: "size" includes the size of the TCP header

void tcp_socket_init(struct tcp_socket *sock);
/****************************************************************/
void tcp_buffers_reset(struct tcp_socket *sock);
/****************************************************************/
si32_t tcp_socket_alloc();
/****************************************************************/
void tcp_socket_free(si32_t sock_id);
/****************************************************************/
bool_t tcp_port_is_used(ui16_t port);
/****************************************************************/
ui16_t tcp_port_alloc();
/****************************************************************/
struct tcp_socket *tcp_find_socket(ui32_t source_ip,
                                   ui16_t source_port,
                                   ui16_t dest_port);
/****************************************************************/
void tcp_set_socket_state(struct tcp_socket *sock,
                          sock_state_t state,
                          clock_t timeout);
/****************************************************************/
void tcp_set_buffer_state(struct tcp_socket *sock,
                          struct tcp_buffer *buf,
                          buf_state_t state);
/****************************************************************/
ret_t tcp_request_push(struct tcp_socket *sock,
                       ui32_t source_ip,
                       ui32_t dest_ip,
                       ui16_t source_port,
                       ui16_t dest_port,
                       ui32_t irs);
/****************************************************************/
struct tcp_request *tcp_request_pop(struct tcp_socket *sock);
/****************************************************************/
void tcp_receive(struct tcp_header *packet,
                 size_t size,
                 ui32_t source_ip,
                 ui32_t dest_ip);
/****************************************************************/
void tcp_callback();
/****************************************************************/
bool_t tcp_socket_timeout(struct tcp_socket *sock);
/****************************************************************/
bool_t tcp_buffer_timeout(struct tcp_socket *sock, struct tcp_buffer *buf);
/****************************************************************/
// WARNING: enough space (12 bytes) must be reserved before address "packet" to
// store TCP pseudo header.
ui16_t tcp_checksum(struct tcp_header *packet,
                    size_t size,
                    ui32_t source_ip,
                    ui32_t dest_ip);
/****************************************************************/
ret_t tcp_send(struct tcp_socket *sock,
               ui32_t seq_num,
               ui32_t ack_num,
               void *data,
               size_t size,
               ui32_t flags);
/****************************************************************/
ret_t tcp_send_ack(struct tcp_socket *sock);
/****************************************************************/
ret_t tcp_send_fin(struct tcp_socket *sock, bool_t again);
/****************************************************************/
void tcp_data_in(struct tcp_socket *sock,
                 ui32_t seq_num,
                 void *data,
                 size_t size);
/****************************************************************/
ret_t tcp_data_ack(struct tcp_socket *sock, ui32_t ack_num);
/****************************************************************/
ret_t tcp_data_out(struct tcp_socket *sock, void *data, size_t size);
/****************************************************************/
size_t tcp_data_push(struct tcp_socket *sock, void *buf, size_t max_size);

#endif
