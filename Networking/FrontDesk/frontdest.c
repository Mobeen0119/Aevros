#include "frontdesk.h"
#include "../../kernel/PCI/pci.h"
#include "../../kernel/io.h"
#include "../../kernel/pic.h"
#include "../../kernel/Memory/kheap.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../Bouncer/bouncer.h"
#include "../mailroom/mailroom.h"
#include "../WatchList/watchlist.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

#define REG_MAC0 0x00
#define REG_RBSTART 0x30
#define REG_CMD 0x37
#define REG_CAPR 0x38
#define REG_IMR 0x3C
#define REG_ISR 0x3E
#define REG_RCR 0x44
#define REG_CONFIG1 0x52
#define REG_TSAD0 0x20
#define REG_TSD0 0x10

#define CMD_RESET 0x10
#define CMD_RX_ENABLE 0x08
#define CMD_TX_ENABLE 0x04
#define CMD_BUFE 0x01

#define ISR_ROK 0x01
#define ISR_TOK 0x04
#define ISR_RXOVW 0x10
#define ISR_RER 0x02

#define RX_BUFFER_SIZE (8192 + 16 + 1500)

static frontdesk_state_t state;
static pci_device_t nic_pci;
static uint8_t *rx_buffer;
static uint16_t rx_read_offset;
static int tx_next_desc;

const frontdesk_state_t *frontdesk_get_state()
{
    return &state;
}

void frontdest_init(void)
{
    memset(&state, 0, sizeof(state));
    nic_pci = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!nic_pci.found)
    {
        state.present = 0;
        return;
    }

    uint16_t io_base = (uint16_t)(nic_pci.bar0 & 0xFFFC);

    state.io_base = io_base;
    state.irq_line = nic_pci.interrupt_line;

    uint32_t command = pci_config_read32(nic_pci.bus, nic_pci.slot, nic_pci.function, 0x04);
    command |= (1 << 2);

    pci_config_write32(nic_pci.bus, nic_pci.slot, nic_pci.function, 0x04, command);

    outb(io_base + REG_CONFIG1, 0x00);

    outb(io_base + REG_CMD, CMD_RESET);

    while ((inb(io_base + REG_CMD) & CMD_RESET) != 0)
        ;

    for (int i = 0; i < 6; i++)
    {
        state.mac[i] = inb(io_base + REG_MAC0 + i);
    }

    rx_buffer = (uint8_t *)kmalloc(RX_BUFFER_SIZE);
    memset(rx_buffer, 0, RX_BUFFER_SIZE);

    rx_read_offset = 0;
    outl(io_base + REG_RBSTART, (uint32_t)(uintptr_t)rx_buffer);

    outw(io_base + REG_IMR, ISR_ROK | ISR_TOK | ISR_RXOVW | ISR_RER);

    outl(io_base + REG_RCR, 0x0E | (1 << 7));

    outb(io_base + REG_CMD, CMD_RX_ENABLE | CMD_TX_ENABLE);

    tx_next_desc = 0;
    state.present = 1;

    kprintf("[FrontDesk] RTL8139 found at PCI %x:%x.%x, io_base=%x, irq=%d, mac=%x:%x:%x:%x:%x:%x\n",
            nic_pci.bus, nic_pci.slot, nic_pci.function, io_base, nic_pci.interrupt_line,
            state.mac[0], state.mac[1], state.mac[2], state.mac[3], state.mac[4], state.mac[5]);
}

int frontdesk_send(const void *data, uint16_t length)
{
    if (!state.present || length > 1792)
        return 0;

    uint32_t tsad = state.io_base + REG_TSAD0 + (tx_next_desc * 4);
    uint32_t tsd = state.io_base + REG_TSD0 + (tx_next_desc * 4);

    outl(tsad, (uint32_t)(uintptr_t)data);
    outl(tsd, (uint32_t)length);

    tx_next_desc = (tx_next_desc + 1) % 4;
    state.packets_send++;
    return 1;
}

void frontdesk_irq_handler(void)
{
    if (!state.present)
        return;

    uint16_t io_base = state.io_base;
    uint16_t status = inw(io_base + REG_ISR);

    if (status & ISR_ROK)
    {
        while (!(inb(io_base + REG_CMD) & CMD_BUFE))
        {

            uint16_t packet_status = *(uint16_t *)(rx_buffer + rx_read_offset);
            uint16_t packet_length = *(uint16_t *)(rx_buffer + rx_read_offset + 2);
            uint8_t *frame = rx_buffer + rx_read_offset + 4;

            if (packet_status & 0x01)
            {
                bounce_verdict_t verdict = bouncer_check(frame, packet_length, state.mac);

                const uint8_t *dest = (packet_length >= 6) ? frame : 0;
                const uint8_t *src = (packet_length >= 12) ? frame + 6 : 0;
                bouncer_log(verdict, packet_length, dest, src);

                if (verdict == BOUNCE_ACCEPT)
                {
                    int flagged = src ? watchlist_observe(src) : 0;

                    if (!flagged)
                    {
                        state.packets_received++;
                        mailroom_deliver(frame, packet_length, state.mac);
                    }
                }
            }
            else
            {
                bounce_log(BOUNCE_REJECT_TOO_SHORT, packet_length, 0, 0);
            }
            rx_read_offset = (uint16_t)((rx_read_offset + packet_length + 4 + 3) & ~3);

            if (rx_read_offset > RX_BUFFER_SIZE - 16)
                rx_read_offset -= (RX_BUFFER_SIZE - 16);

            outw(io_base + REG_CAPR, (uint16_t)(rx_read_offset - 16));
        }
    }
    if (status & (ISR_RXOVW | ISR_RER))
        state.rx_errors++;

    outw(io_base + REG_ISR,status);
}
