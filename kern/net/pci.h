#ifndef _PCI_H_
#define _PCI_H_

#include <kernel/errno.h>
#include <kernel/types.h>

/** PCI device structure **/

struct pci_device
{
	ui8_t bus;
	ui8_t dev;
	ui8_t func;
};

/** PCI I/O ports **/

#define PCI_CONFIG_ADDRESS	0xcf8
#define PCI_CONFIG_DATA		0xcfc

/** PCI registers **/

#define PCI_REG_VENDOR_ID		0x00
#define PCI_REG_DEVICE_ID		0x02
#define PCI_REG_COMMAND			0x04
#define PCI_REG_STATUS			0x06
#define PCI_REG_REV_ID			0x08
#define PCI_REG_PROG_IF			0x09
#define PCI_REG_SUBCLASS		0x0a
#define PCI_REG_CLASS_CODE		0x0b
#define PCI_REG_CACHE_LINE_SIZE		0x0c
#define PCI_REG_LATENCY_TIMER		0x0d
#define PCI_REG_HEADER_TYPE		0x0e
#define PCI_REG_BIST			0x0f
#define PCI_REG_BAR0			0x10
#define PCI_REG_BAR1			0x14
#define PCI_REG_BAR2			0x18
#define PCI_REG_BAR3			0x1c
#define PCI_REG_BAR4			0x20
#define PCI_REG_BAR5			0x24
#define PCI_REG_CARDBUS_SYS_POINTER	0x28
#define PCI_REG_SUBSYSTEM_VENDOR_ID	0x2c
#define PCI_REG_SUBSYSTEM_ID		0x2e
#define PCI_REG_EXPANSION_ROM_ADDRESS	0x30
#define PCI_REG_CAPABILITIES_POINTER	0x34
#define PCI_REG_INTERRUPT_LINE		0x3c
#define PCI_REG_INTERRUPT_PIN		0x3d
#define PCI_REG_MIN_GRANT		0x3e
#define PCI_REG_MAX_LATENCY		0x3f

/** Functions **/

ui8_t pci_read_byte(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg);
/****************************************************************/
ui16_t pci_read_word(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg);
/****************************************************************/
ui32_t pci_read_dword(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg);
/****************************************************************/
void pci_write_word(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg, ui16_t word);
/****************************************************************/
ret_t pci_find_device(ui16_t vendor_id,
                      ui16_t device_id,
                      struct pci_device *pci_dev);

#endif
