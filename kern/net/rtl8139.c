/****************************************************************
 * rtl8139.c                                                    *
 *                                                              *
 *    RTL8139 driver.                                           *
 *                                                              *
 ****************************************************************/

#define _RTL8139_C_
#include <config.h>
#include <kernel/io.h>
#include <kernel/isr.h>
#include <kernel/libc.h>
#include <kernel/panic.h>
#include <net/ether.h>
#include <net/pci.h>

#include "rtl8139.h"

ui16_t io_base;
ui8_t rx_buffer[8192 + 16 + 1536] __attribute__((aligned(4))) = { 0 };
ui16_t rx_cur = 0;
struct rtl8139_tx_desc tx_desc[4];
ui8_t first_tx_desc = 0, cur_tx_desc = 0;

/**
 * rtl8139_init
 */

void rtl8139_init()
{
	struct pci_device pci_dev;
	ui8_t int_line;
	ui16_t cr;

	/** Look for the device on the bus. **/

	if(pci_find_device(0x10ec, 0x8139, &pci_dev) != OK)
	{
		panic("device not found");
	}

	/** Read the I/O base address. **/

	io_base = pci_read_word(pci_dev.bus,
	                        pci_dev.dev,
	                        pci_dev.func,
	                        PCI_REG_BAR0);

	if(!(io_base & 1))
	{
		panic("bar0 is not an i/o address");
	}

	io_base &= ~1;

	#ifdef DEBUG_RTL8139
	printk("i/o base: %x\n", io_base);
	#endif

	/** Read the MAC address. **/

	local_hwaddr[0] = inb(io_base + IDR_BASE);
	local_hwaddr[1] = inb(io_base + IDR_BASE + 1);
	local_hwaddr[2] = inb(io_base + IDR_BASE + 2);
	local_hwaddr[3] = inb(io_base + IDR_BASE + 3);
	local_hwaddr[4] = inb(io_base + IDR_BASE + 4);
	local_hwaddr[5] = inb(io_base + IDR_BASE + 5);

	#ifdef DEBUG
	printk("mac address: %x, %x, %x, %x, %x, %x\n",
	       local_hwaddr[0],
	       local_hwaddr[1],
	       local_hwaddr[2],
	       local_hwaddr[3],
	       local_hwaddr[4],
	       local_hwaddr[5]);
	#endif

	/** Read the interrupt line and register the NIC interrupt handler. **/

	int_line = pci_read_byte(pci_dev.bus,
	                         pci_dev.dev,
	                         pci_dev.func,
	                         PCI_REG_INTERRUPT_LINE);

	if(int_line == 0x09)
	{
		_isr_0x09_irq_handler = rtl8139_isr;
	}
	else if(int_line == 0x0a)
	{
		_isr_0x0a_irq_handler = rtl8139_isr;
	}
	else if(int_line == 0x0b)
	{
		_isr_0x0b_irq_handler = rtl8139_isr;
	}
	else
	{
		panic("interrupt line is %x", int_line);
	}

	/** Enable bus mastering. **/

	cr = pci_read_word(pci_dev.bus,
	                   pci_dev.dev,
	                   pci_dev.func,
	                   PCI_REG_COMMAND);
	cr |= 4;
	pci_write_word(pci_dev.bus,
	               pci_dev.dev,
	               pci_dev.func,
	               PCI_REG_COMMAND,
	               cr);

	/** Finally, reset the card. **/

	rtl8139_reset();
}

/**
 * rtl8139_reset
 */

void rtl8139_reset()
{
	ui16_t imr;
	ui32_t rcr, tcr;

	/** Read IMR. **/

	imr = inw(io_base + IMR);

	/** Reset the card. **/

	outb(io_base + CONFIG1, 0);
	outb(io_base + CR, CR_RST);

	while(inb(io_base + CR) & CR_RST);

	/** Initialize receiver. **/

	rcr = /*(1 << 0) // accept all packets
	    |*/ (1 << 1) // accept physical match packets
	    //| (1 << 2) // accept multicast packets
	    | (1 << 3) // accept broadcast packets
	    | (1 << 4) // accept runt packets
	    | (1 << 5) // accept error packets
	    | (1 << 7) // enable wrap
	    | (6 << 8) // max DMA burst size: 1024 bytes
	    | (0 << 11) // receive buffer size: 8192 + 16 bytes
		/** NOTE: TacOS used (6 << 14) **/
	    | (6 << 13) // FIFO threshold: 1024 bytes
	    | (1 << 24); // rx threshold: 1/16

	outb(io_base + CR, CR_RE | CR_TE);
	outl(io_base + RCR, rcr);
	outl(io_base + RBSTART, rx_buffer);
	//outw(io_base + CAPR, 0);
	imr |= IR_ROK | IR_RER | IR_RXOVW | IR_FOVW;

	/** Initialize transmitter. **/

	tcr = (6 << 8) // max DMA burst size: 1024 bytes
	    | (3 << 24); // interframe gap time : 9.6 µs for 10 Mbps,
	                 // 940 ns for 100 Mbps

	outl(io_base + TCR, tcr);
	imr |= IR_TOK | IR_TER;

	/** Update IMR. **/

	outw(io_base + IMR, imr);

	/** Enable receiving and transmitting. **/

	outb(io_base + CR, CR_RE | CR_TE);

	/** Reset global variables. **/

	rx_cur = 0;
	first_tx_desc = cur_tx_desc = 0;
	free_tx_desc = 4;
}

/**
 * rtl8139_send
 */

ret_t rtl8139_send(ui8_t *packet, size_t packet_len)
{
	static int counter = 0;

	/*if(counter % 4)
	{
		counter++;
		return OK;
	}*/

	#ifdef DEBUG_RTL8139
	printk("rtl8139_send: (%x free buffers, %x)\n", free_tx_desc, counter);
	#endif

	/** Return immediately if there is no TX descriptor left. **/

	if(!free_tx_desc)
	{
		return -ENOMEM;
	}

	/** Check packet length. **/

	if(packet_len > 1516)
	{
		return -EINVAL;
	}

	/** Fill the current TX descriptor. **/

	memcpy(tx_desc[cur_tx_desc].buf, packet, packet_len);
	tx_desc[cur_tx_desc].packet_len = (ui16_t)packet_len;

	/** Issue it. **/

	outl(io_base + TSAD_BASE + (cur_tx_desc << 2),
	     tx_desc[cur_tx_desc].buf);
	outl(io_base + TSD_BASE + (cur_tx_desc << 2),
	     tx_desc[cur_tx_desc].packet_len);

	/** Update cur_tx_desc and free_tx_desc. **/

	#ifdef DEBUG_RTL8139
	printk(">>> cur_tx_desc: %x\n", cur_tx_desc);
	#endif

	cur_tx_desc = (cur_tx_desc + 1) % 4;
	free_tx_desc--;

	#ifdef DEBUG_RTL8139
	printk("free_tx_desc = %x\n", free_tx_desc);
	#endif

	counter++;

	return OK;
}

/**
 * rtl8139_rx_isr
 */

void rtl8139_rx_isr()
{
	ui8_t cr;
	struct rtl8139_rx_header *header;
	ui8_t *data;

	cr = inb(io_base + CR);

	while(!(cr & CR_BUFE))
	{
		header = (struct rtl8139_rx_header*)(rx_buffer
		                                     + (rx_cur % 8192));
		data = (ui8_t*)header + 4;

		if(header->runt || header->fae || header->crc || header->longp)
		{
			#ifdef DEBUG_RTL8139
			printk("packet is corrupted\n");
			#endif

			rtl8139_reset();
		}
		else
		{
			/** Dump the packet. **/

			/*#ifdef DEBUG_RTL8139
			ui16_t i;

			for(i = 0; i < header->packet_len; i += 4)
			{
				if(!(i % 16))
				{
					printk("\n");
				}

				printk("%x ", *(ui32_t*)(data + i));

			}

			printk("\n");
			#endif*/

			ether_receive(data, header->packet_len - 4);
		}

		/*#ifdef DEBUG_RTL8139
		printk("rx_buffer: %x | rx_cur: %x\n", rx_buffer, rx_cur);
		printk("packet length: %x\n", header->packet_len);
		//delay(5000);
		#endif*/

		if(!header->packet_len)
		{
			#ifdef DEBUG_RTL8139
			printk("null packet length\n");
			#endif
			rtl8139_reset();
			return;
		}

		rx_cur = (rx_cur + header->packet_len + 4 + 3) & ~3;
		outw(io_base + CAPR, rx_cur - 0x10);

		cr = inb(io_base + CR);
	}
}

/**
 * rtl8139_tx_isr
 */

void rtl8139_tx_isr()
{
	ui32_t tsd;

	while(( (tsd = inl(io_base + TSD_BASE + (first_tx_desc << 2))) & TSD_TOK)
	    && (tsd & TSD_OWN)
	    && free_tx_desc < 4)
	{
		#ifdef DEBUG_RTL8139
		printk("packet %x sent\n", first_tx_desc);
		#endif
		first_tx_desc = (first_tx_desc + 1) % 4;
		free_tx_desc++;
	}

	/*#ifdef DEBUG_RTL8139
	printk("free_tx_desc: %x\n", free_tx_desc);
	printk("first_tx_desc: %x\n", first_tx_desc);
	printk("tsd: %x\n", tsd);
	#endif*/
}

/**
 * rtl8139_isr
 */

void rtl8139_isr()
{
	ui16_t isr = inw(io_base + ISR);

	#ifdef DEBUG_RTL8139
	printk("\nnetwork card interrupt (isr: %x) at tic %x\n", isr, tics);
	#endif

	/** Check transmission state. **/

	if(isr & (IR_TOK | IR_TER))
	{
		if(isr & IR_TOK)
		{
			#ifdef DEBUG_RTL8139
			printk("transmission ok\n");
			#endif
			rtl8139_tx_isr();
		}
		else if(isr & IR_TER)
		{
			#ifdef DEBUG_RTL8139
			printk("transmission error\n");
			#endif
			rtl8139_reset();
		}

		outw(io_base + ISR, IR_TOK | IR_TER);
	}

	/** Check reception state. **/

	if((isr & IR_ROK)
	|| (isr & IR_RER)
	|| (isr & IR_RXOVW)
	|| (isr & IR_FOVW))
	{
		#ifdef DEBUG_RTL8139
		if(isr & IR_ROK)
		{
			printk("reception ok\n");
		}
		else if(isr & IR_RER)
		{
			printk("reception error\n");
		}
		else if(isr & IR_RXOVW)
		{
			printk("overflow\n");
		}
		else if(isr & IR_FOVW)
		{
			printk("fifo overflow\n");
		}
		#endif

		if(isr & IR_ROK)
		{
			rtl8139_rx_isr();
		}

		outw(io_base + ISR, IR_ROK | IR_RER | IR_RXOVW | IR_FOVW);
	}

	#ifdef DEBUG_RTL8139
	printk("\n");
	#endif
}
