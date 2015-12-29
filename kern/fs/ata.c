/****************************************************************
 * ata.c                                                        *
 *                                                              *
 *    ATA hard disks read/write.                                *
 *                                                              *
 ****************************************************************/

#include <config.h>
#include <kernel/errno.h>
#include <kernel/io.h>
#ifdef DEBUG
#include <kernel/printk.h>
#endif

#include "ata.h"

/**
 * ata_init
 */

void ata_init(ui8_t ctl)
{
	ui8_t devctl_reg = (ctl == 0) ? 0x3f6 : 0x376;

	/** No interrupts. **/

		outb(devctl_reg, ATA_DEVCTL_NIEN);

	/** Software reset. **/

	outb(devctl_reg, ATA_DEVCTL_SRST);

	/** Delay by reading the status register. **/

	inb(devctl_reg);
	inb(devctl_reg);
	inb(devctl_reg);
	inb(devctl_reg);
}

/**
 * ata_read_write
 */

ret_t ata_read_write(ui8_t ctl,
                     ui8_t slave,
                     void *buf,
                     ui32_t lba,
                     bool_t write)
{
	static ui8_t prev_ctl = 2, prev_slave = 2;
	ui16_t base = (ctl == 0) ? 0x1f0 : 0x170;
	#ifdef ATA_CACHE_FLUSH
	ui8_t altstatus_reg = (ctl == 0) ? 0x3f6 : 0x376;
	#endif
	ui8_t status;
	ui16_t i, word;
	bool_t ready, error, writing;

	#ifdef DEBUG
	printk("ata i/o operation\n");
	#endif

	if(prev_ctl != ctl || prev_slave != slave)
	{
		/** Select the drive and give the 4 highest bits of the LBA. **/

		outb(base + 6, 0xe0 | slave << 4/* | lba >> 24*/);

		/** Wait some time for the selection to complete. **/

		#ifdef ATA_SEL_PIO
		outb(base + 1, 0x00);
		outb(base + 1, 0x00);
		outb(base + 1, 0x00);
		outb(base + 1, 0x00);
		#else
		delay(ATA_SEL_DELAY);
		#endif

		prev_ctl = ctl;
		prev_slave = slave;
	}

	/** Number of sectors to read/write: 1. **/

	outb(base + 2, 0x01);

	/** Give the 3 lowest bytes of the LBA. **/

	outb(base + 3, (ui8_t)lba);
	outb(base + 4, (ui8_t)(lba >> 8));
	outb(base + 5, (ui8_t)(lba >> 16));

	/** Send the command. **/

	if(write)
	{
		outb(base + 7, ATA_CMD_WRITE);
	}
	else
	{
		outb(base + 7, ATA_CMD_READ);
	}

	/** Poll until the drive is ready for transfer. **/

	do
	{
		status = inb(base + 7);
		ready = !(status & ATA_STATUS_BSY)
		      && (status & ATA_STATUS_DRQ);
		error = status & ATA_STATUS_ERR;
	} while(!ready && !error);

	if(error)
	{
		return -EIO;
	}

	/** Perform data transfer. **/

	if(write)
	{
		for(i = 0; i < 256; i++)
		{
			word = ((ui16_t*)buf)[i];
			outw(base, word);
			asm volatile("jmp next \n\
			              nop \n\
			              next:");
		}

		/** Wait for the drive to receive the data. **/

		do
		{
			status = inb(base + 7);
			writing = (status & ATA_STATUS_BSY)
			       || (status & ATA_STATUS_DRQ);
			error = status & ATA_STATUS_ERR;
		} while(writing && !error);

		if(error)
		{
			return -EIO;
		}
	}
	else
	{
		for(i = 0; i < 256; i++)
		{
			word = inw(base);
			((ui16_t*)buf)[i] = word;
		}
	}

	#ifdef ATA_CACHE_FLUSH

	/** If we have just written to the disk, perform a cache flush. **/

	if(write)
	{
		#ifdef DEBUG
		printk("cache flush\n");
		#endif

		outb(base + 7, ATA_CMD_CACHE_FLUSH);

		inb(altstatus_reg);
		inb(altstatus_reg);
		inb(altstatus_reg);
		inb(altstatus_reg);

		while(inb(base + 7) & ATA_STATUS_BSY);
	}

	#endif

	/** Wait some time. **/

	/*inb(altstatus_reg);
	inb(altstatus_reg);
	inb(altstatus_reg);
	inb(altstatus_reg);*/

	return OK;
}
