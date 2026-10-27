#include "e1000.h"
#include "../../../kernel/PCI/pci.h"
#include "../../../kernel/Paging/paging.h"
#include "../../../kernel/Process/task.h"
#include "../../../Include/critical.h"
#include "../../../Lib/string.h"
#include "../../../Lib/kprintf.h"
#include "../../../Networking/NetWatch/net_watch.h"
#include "../../../Networking/NetWatch/net_observe.h"

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E

#define REG_CTRL 0x0000
#define REG_STATUS 0x0008
#define REG_ICR 0x00C0
#define REG_IMS 0x00D0
#define REG_IMC 0x00D8

#define REG_RCTL 0x0100
#define REG_TCTL 0x0400
#define REG_RDBAL 0x2800
#define REG_RDBAH 0x2804
#define REG_RDLEN 0x2808

#define REG_RDH 0x2810
#define REG_RDT 0x2818
#define REG_TDBAL 0x3800
#define REG_TDBAH 0x3804
#define REG_TDLEN 0x3808

#define REG_TDH 0x3810
#define REG_TDT 0x3818
#define REG_MTA 0x5200
#define REG_RAL0 0x5400
#define REG_RAH0 0x5404

#define CTRL_RST (1u << 26)
#define CTRL_SLU (1u << 6)
#define STATUS_LU (1u << 1)
#define RCTL_EN (1u << 1)
#define RCTL_MPE (1u << 4)

#define RCTL_BAM (1u << 15)
#define TCTL_EN (1u << 1)
#define TCTL_PSP (1u << 3)
#define IRQ_TXDW (1u << 0)

#define IRQ_LSC (1u << 2)
#define IRQ_RXO (1u << 6)
#define IRQ_RXT0 (1u << 7)

#define DESC_DD 0x01
#define TX_CMD_EOP 0x01
#define TX_CMD_IFCS 0x02
#define TX_CMD_RS 0x08

#define RING_LEN 16
#define BUF_LEN 2048
#define MMIO_PAGES 32

typedef struct __attribute__((packed))
{
    uint64_t addr;
    uint16_t length;
    uint8_t cso;

    uint8_t cmd;
    uint8_t status;
    uint8_t errors;

    uint16_t special;

} e1000_desc_t;

static volatile uint8_t *mmio;
static e1000_desc_t rx_ring[RING_LEN] __attribute__((aligned(16)));
static e1000_desc_t tx_ring[RING_LEN] __attribute__((aligned(16)));

static uint8_t rx_buf[RING_LEN][BUF_LEN] __attribute__((aligned(16)));
static uint8_t tx_buf[RING_LEN][BUF_LEN] __attribute__((aligned(16)));
static frontdesk_state_t *st;

static uint32_t rx_next;
static uint32_t rx_tail;
static uint32_t tx_tail;

static uint32_t tx_clean;
static uint32_t tx_inflight;

static inline uint32_t reg_read(uint32_t off)
{
    return *(volatile uint32_t *)(mmio + off);
}

static inline void reg_write(uint32_t off, uint32_t value)
{
    *(volatile uint32_t *)(mmio + off) = value;
}

static void reclaim_tx(void)
{
    while (tx_inflight > 0 && (tx_ring[tx_clean].status & DESC_DD))
    {
        tx_ring[tx_clean].status = 0;
        tx_clean = (tx_clean + 1) % RING_LEN;
        tx_inflight--;
    }
}

int e1000_probe(frontdesk_state_t *dev_state)
{
    st = dev_state;

    pci_device_t dev = pci_find_device(E1000_VENDOR_ID, E1000_DEVICE_ID);
    if (!dev.found)
    {
        kprintf("[FrontDesk] e1000 not found on PCI bus\n");
        return 0;
    }

    uint32_t base = dev.bar0 & 0xFFFFFFF0u;
    for (uint32_t p = 0; p < MMIO_PAGES; p++)
        map_page(base + p * 4096, base + p * 4096, PAGE_PRESENT | PAGE_WRITE);

    mmio = (volatile uint8_t *)(uintptr_t)base;

    uint32_t command = pci_config_read32(dev.bus, dev.slot, dev.function, 0x04);
    pci_config_write32(dev.bus, dev.slot, dev.function, 0x04, command | 0x06);

    reg_write(REG_IMC, 0xFFFFFFFFu);
    reg_write(REG_CTRL, reg_read(REG_CTRL) | CTRL_RST);

    uint32_t deadline = get_ticks() + 200;

    while (reg_read(REG_CTRL) & CTRL_RST)
    {
        if (get_ticks() > deadline)
        {
            kprintf("[FrontDesk] e1000 reset never completed\n");
            return 0;
        }
    }

    reg_write(REG_IMC, 0xFFFFFFFFu);
    reg_write(REG_CTRL, reg_read(REG_CTRL) | CTRL_SLU);

    for (int i = 0; i < 128; i++)
        reg_write(REG_MTA + i * 4, 0);

    uint32_t ral = reg_read(REG_RAL0);
    uint32_t rah = reg_read(REG_RAH0);

    st->mac[0] = (uint8_t)(ral);
    st->mac[1] = (uint8_t)(ral >> 8);
    st->mac[2] = (uint8_t)(ral >> 16);
    st->mac[3] = (uint8_t)(ral >> 24);
    st->mac[4] = (uint8_t)(rah);
    st->mac[5] = (uint8_t)(rah >> 8);

    memset(rx_ring, 0, sizeof(rx_ring));
    memset(tx_ring, 0, sizeof(tx_ring));

    for (int i = 0; i < RING_LEN; i++)
    {
        rx_ring[i].addr = (uint64_t)(uintptr_t)rx_buf[i];
        tx_ring[i].addr = (uint64_t)(uintptr_t)tx_buf[i];
    }

    reg_write(REG_RDBAL, (uint32_t)(uintptr_t)rx_ring);
    reg_write(REG_RDBAH, 0);
    reg_write(REG_RDLEN, RING_LEN * sizeof(e1000_desc_t));

    reg_write(REG_RDH, 0);
    rx_next = 0;
    rx_tail = RING_LEN - 1;

    reg_write(REG_RDT, rx_tail);
    reg_write(REG_RCTL, RCTL_EN | RCTL_BAM | RCTL_MPE);

    reg_write(REG_TDBAL, (uint32_t)(uintptr_t)tx_ring);
    reg_write(REG_TDBAH, 0);
    reg_write(REG_TDLEN, RING_LEN * sizeof(e1000_desc_t));

    reg_write(REG_TDH, 0);
    reg_write(REG_TDT, 0);

    tx_tail = 0;
    tx_clean = 0;
    tx_inflight = 0;
    reg_write(REG_TCTL, TCTL_EN | TCTL_PSP | (0x0Fu << 4) | (0x40u << 12));

    reg_write(REG_IMS, IRQ_TXDW | IRQ_LSC | IRQ_RXO | IRQ_RXT0);

    st->irq_line = dev.interrupt_line;
    st->present = 1;

    netwatch_set_link((reg_read(REG_STATUS) & STATUS_LU) ? 1 : 0);

    kprintf("[FrontDesk] e1000 found at PCI %x:%x.%x, mmio=%x, irq=%d, mac=%x:%x:%x:%x:%x:%x\n",
            dev.bus, dev.slot, dev.function, base, dev.interrupt_line, st->mac[0], st->mac[1], st->mac[2], st->mac[3], st->mac[4], st->mac[5]);
    return 1;
}

int e1000_send(const void *data, uint16_t length)
{
    if (length == 0 || length > BUF_LEN)
        return 0;

    uint32_t flags = critical_enter();

    reclaim_tx();

    if (tx_inflight == RING_LEN)
    {
        st->tx_drops++;
        critical_leave(flags);
        return 0;
    }

    memcpy(tx_buf[tx_tail], data, length);
    tx_ring[tx_tail].length = length;
    tx_ring[tx_tail].cso = 0;
    tx_ring[tx_tail].cmd = TX_CMD_EOP | TX_CMD_IFCS | TX_CMD_RS;
    tx_ring[tx_tail].status = 0;

    tx_tail = (tx_tail + 1) % RING_LEN;
    tx_inflight++;

    st->packets_send++;
    reg_write(REG_TDT, tx_tail);

    critical_leave(flags);

    netobserve_capture(NETWATCH_TX, data, length);
    return 1;
}

void e1000_irq(void)
{
    uint32_t causes = reg_read(REG_ICR);

    if (causes & IRQ_RXO)
        st->rx_errors++;

    if (causes & IRQ_LSC)
        netwatch_set_link((reg_read(REG_STATUS) & STATUS_LU) ? 1 : 0);

    while (rx_ring[rx_next].status & DESC_DD)
    {
        uint16_t len = rx_ring[rx_next].length;
        uint8_t status = rx_ring[rx_next].status;
        uint8_t errors = rx_ring[rx_next].errors;

        if ((status & 0x02) && errors == 0 && len > 0)
            frontdesk_deliver(rx_buf[rx_next], len);

        rx_ring[rx_next].status = 0;
        rx_tail = rx_next;
        rx_next = (rx_next + 1) % RING_LEN;

        reg_write(REG_RDT, rx_tail);
    }

    reclaim_tx();
}