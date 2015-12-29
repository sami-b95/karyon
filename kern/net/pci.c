/****************************************************************
 * pci.c                                                        *
 *                                                              *
 *    PCI functions.                                            *
 *                                                              *
 ****************************************************************/

#include <kernel/io.h>

#include "pci.h"

/**
 * pci_read_byte
 */

ui8_t pci_read_byte(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg)
{
	ui32_t io_addr = 0x80000000
	               | (bus << 16)
	               | (dev << 11)
	               | (func << 8)
	               | (reg & 0xfc);

	outl(PCI_CONFIG_ADDRESS, io_addr);

	return inb(PCI_CONFIG_DATA + (reg & 3));
}

/**
 * pci_read_word
 */

ui16_t pci_read_word(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg)
{
	ui32_t io_addr = 0x80000000
	               | (bus << 16)
	               | (dev << 11)
	               | (func << 8)
	               | (reg & 0xfc);

	outl(PCI_CONFIG_ADDRESS, io_addr);

	return inw(PCI_CONFIG_DATA + (reg & 2));
}

/**
 * pci_read_dword
 */

ui32_t pci_read_dword(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg)
{
	ui32_t io_addr = 0x80000000
	               | (bus << 16)
	               | (dev << 11)
	               | (func << 8)
	               | (reg & 0xfc);

	outl(PCI_CONFIG_ADDRESS, io_addr);

	return inl(PCI_CONFIG_DATA);
}

/**
 * pci_write_word
 */

void pci_write_word(ui8_t bus, ui8_t dev, ui8_t func, ui8_t reg, ui16_t word)
{
	ui32_t io_addr = 0x80000000
	               | (bus << 16)
	               | (dev << 11)
	               | (func << 8)
	               | (reg & 0xfc);

	outl(PCI_CONFIG_ADDRESS, io_addr);
	outw(PCI_CONFIG_DATA + (reg & 2), word);
}

/**
 * pci_find_device
 */

ret_t pci_find_device(ui16_t vendor_id,
                      ui16_t device_id,
                      struct pci_device *pci_dev)
{
	ui16_t vend_id, dev_id;

	pci_dev->bus = 0;

	/** For each device connected to the bus, check vendor ID and device
	    ID. **/

	do
	{
		for(pci_dev->dev = 0; pci_dev->dev < 32; pci_dev->dev++)
		{
			for(pci_dev->func = 0;
			    pci_dev->func < 8;
			    pci_dev->func++)
			{
				vend_id = pci_read_word(pci_dev->bus,
				                        pci_dev->dev,
				                        pci_dev->func,
				                        PCI_REG_VENDOR_ID);

				dev_id = pci_read_word(pci_dev->bus,
				                       pci_dev->dev,
				                       pci_dev->func,
				                       PCI_REG_DEVICE_ID);

				if(vend_id == vendor_id && dev_id == device_id)
				{
					return OK;
				}
			}
		}
	} while(++pci_dev->bus);

	return -ENXIO;
}
