#ifndef _ATA_H_
#define _ATA_H_

#include <kernel/types.h>

/** Some bits for the status registers (regular and alternate). **/

#define ATA_STATUS_ERR	0x01
#define ATA_STATUS_DRQ	0x08
#define ATA_STATUS_BSY	0x80

/** Some bits for the device control register. **/

#define ATA_DEVCTL_NIEN	0x02
#define ATA_DEVCTL_SRST	0x04

/** Some commands. **/

#define ATA_CMD_READ		0x20
#define ATA_CMD_WRITE		0x30
#define ATA_CMD_CACHE_FLUSH	0xe7

/** Functions. **/

void ata_init(ui8_t ctl);
/****************************************************************/
ret_t ata_read_write(ui8_t ctl,
                     ui8_t slave,
                     void *buf,
                     ui32_t lba,
                     bool_t write);

#endif
