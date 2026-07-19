#ifndef PCI_H
#define PCI_H

#include <stdint.h>

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint32_t bar0;
    uint8_t interrupt_line;
    int found;
}pci_device_t;

uint32_t pci_config_read32(uint32_t bus,uint8_t slot,uint8_t func,uint8_t offset);
uint16_t pci_config_read16(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset);
uint8_t pci_config_read8(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset);
void pci_config_write32(uint8_t bus,uint8_t slot,uint8_t func,uint8_t offset);
pci_device_t pci_find_device(uint16_t vendor_id,uint16_t device_id);

#endif