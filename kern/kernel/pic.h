#ifndef _PIC_H_
#define _PIC_H_

/** Base I/O addresses for master and slave PIC **/

#define PIC1_BASE	0x20
#define PIC2_BASE	0xa0

/** ICW1 flags **/

#define PIC_ICW1_DEFAULT	0x10
#define PIC_ICW1_WITH_ICW4	0x01
#define PIC_ICW1_ONE_CONTROLLER	0x02
#define PIC_ICW1_CASCADE	0x08

/** ICW4 flags **/

#define PIC_ICW4_DEFAULT		0x01
#define PIC_ICW4_AUTO_EOI		0x02
#define PIC_ICW4_BUFFERED_MASTER	0x04
#define PIC_ICW4_BUFFERED		0x08
#define PIC_ICW4_FULLY_NESTER		0x10

/** PIC initialization function **/

void pic_init();

#endif
