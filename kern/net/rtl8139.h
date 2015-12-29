#ifndef _RTL8139_H_
#define _RTL8139_H_

#include <kernel/types.h>

/** Transmit decriptor structure **/

struct rtl8139_tx_desc
{
	ui8_t buf[16 + 1500] __attribute__((aligned(4)));
	ui16_t packet_len;
};

/** Received packet header structure **/

struct rtl8139_rx_header
{
	ui16_t rok:1;
	ui16_t fae:1;
	ui16_t crc:1;
	ui16_t longp:1;
	ui16_t runt:1;
	ui16_t ise:1;
	ui16_t :7;
	ui16_t bar:1;
	ui16_t pam:1;
	ui16_t mar:1;
	ui16_t packet_len;
};

/** Registers **/

#define IDR_BASE	0x00
#define TSD_BASE	0x10
#define TSAD_BASE	0x20
#define RBSTART		0x30
#define CR		0x37
#define CAPR		0x38
#define IMR		0x3c
#define ISR		0x3e
#define TCR		0x40
#define RCR		0x44
#define CONFIG1		0x52

/** TSD flags **/

#define TSD_OWN	0x00002000
#define TSD_TOK	0x00008000

/** CR flags **/

#define CR_BUFE	0x01
#define CR_TE	0x04
#define CR_RE	0x08
#define CR_RST	0x10

/** ISR/IMR flags **/

#define IR_ROK		0x0001
#define IR_RER		0x0002
#define IR_TOK		0x0004
#define IR_TER		0x0008
#define IR_RXOVW	0x0010
#define IR_FOVW		0x0040

/** Global variables. **/

#ifdef _RTL8139_C_
count_t free_tx_desc = 4;
#else
extern count_t free_tx_desc;
#endif

/** Functions **/

void rtl8139_init();
/****************************************************************/
void rtl8139_reset();
/****************************************************************/
ret_t rtl8139_send(ui8_t *packet, size_t packet_len);
/****************************************************************/
void rtl8139_rx_isr();
/****************************************************************/
void rtl8139_tx_isr();
/****************************************************************/
void rtl8139_isr();

#endif
