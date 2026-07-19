#include "pci.h"
#include "../io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

static uint32_t pci_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    return (uint32_t)((1u << 31) |
                      ((uint32_t)bus << 16) |
                      ((uint32_t)(slot & 0x1F) << 11) |
                      ((uint32_t)(func & 0x07) << 8) |
                      (offset & 0xFC));
}
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, slot, func, offset));
    return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
{
    uint32_t val = pci_config_read32(bus, slot, func, offset & 0xFC);
    return (uint16_t)((val >> ((offset & 2) * 8)) & 0xFFFF);
}

void pci_config_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value)
{
    outl(PCI_CONFIG_ADDRESS, pci_address(bus, slot, func, offset));
    outl(PCI_CONFIG_DATA, value);
}

pci_device_t pci_find_device(uint16_t vendor_id, uint16_t device_id)
{
    pci_device_t dev;
    dev.found = 0;
    for (uint32_t bus = 0; bus < 256; bus++)
    {
        for (uint32_t slot = 0; slot < 32; slot++)
        {
            uint16_t vendor = pci_config_read16((uint8_t)bus, (uint8_t)slot, 0, 0x00);

            if (vendor == 0xFFFF)
                continue;
            for (uint32_t func = 0; func < 8; func++)
            {
                uint16_t v = pci_config_read16((uint8_t)bus, (uint8_t)slot, (uint8_t)func, 0x00);
                if (v != vendor_id)
                    continue;

                uint16_t d = pci_config_read16((uint8_t)bus, (uint8_t)slot, (uint8_t)func, 0x02);
                if (d != device_id)
                    continue;
                
                dev.bus = (uint8_t)bus;
                dev.slot = (uint8_t)slot;

                dev.function = (uint8_t)func;
                dev.vendor_id = v;
                dev.device_id = d;

                dev.bar0 = pci_config_read32((uint8_t)bus, (uint8_t)slot, (uint8_t)func, 0x10);
                dev.interrupt_line = pci_config_read8((uint8_t)bus, (uint8_t)slot, (uint8_t)func, 0x3C);
                dev.found = 0;

                return dev;
            }
        }
    }
    return dev;
}