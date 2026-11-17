#include "ata.h"
#include "../../kernel/io.h"

#define ATA_DATA 0x1F0
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LO 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_CMD 0x1F7

#define ST_ERR 0x01
#define ST_DRQ 0x08
#define ST_DF 0x20
#define ST_BSY 0x80

#define CMD_IDENTIFY 0xEC
#define CMD_READ 0x20
#define CMD_WRITE 0x30
#define CMD_FLUSH 0xE7

static uint32_t sectors;
static int present;

static int wait_ready(void)
{
    for (uint32_t i = 0; i < 1000000; i++)
    {
        uint8_t s = inb(ATA_STATUS);
        if (!(s & ST_BSY))
            return (s & (ST_ERR | ST_DF)) ? -1 : 0;
    }
    return -1;
}

static int wait_drq(void)
{
    for (uint32_t i = 0; i < 1000000; i++)
    {
        uint8_t s = inb(ATA_STATUS);

        if (s & ST_ERR || s & ST_DF)
            return -1;

        if (!(s & ST_BSY) && (s & ST_DRQ))
            return 0;
    }
    return -1;
}

int ata_init(void)
{
    present = 0;
    sectors = 0;

    outb(ATA_DRIVE, 0xA0);
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LO, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HI, 0);
    outb(ATA_CMD, CMD_IDENTIFY);

    if (inb(ATA_STATUS) == 0)
        return -1;

    if (wait_ready() < 0)
        return -1;

    if (inb(ATA_LBA_MID) != 0 || inb(ATA_LBA_HI) != 0)
        return -1;

    if (wait_drq() < 0)
        return -1;

    uint16_t id[256];
    for (int i = 0; i < 256; i++)
        id[i] = inw(ATA_DATA);

    sectors = (uint32_t)id[60] | ((uint32_t)id[61] << 16);
    present = sectors > 0;

    return present ? 0 : -1;
}

uint32_t ata_sector_count(void)
{
    return present ? sectors : 0;
}

int ata_read(uint32_t lba, uint32_t count, uint8_t *buf)
{
    if (!present || lba + count > sectors || lba >= 0x10000000)
        return -1;

    for (uint32_t s = 0; s < count; s++)
    {
        uint32_t l = lba + s;
        outb(ATA_DRIVE, 0xE0 | ((l >> 24) & 0x0F));
        outb(ATA_SECCOUNT, 1);
        outb(ATA_LBA_LO, (uint8_t)l);
        outb(ATA_LBA_MID, (uint8_t)(l >> 8));
        outb(ATA_LBA_HI, (uint8_t)(l >> 16));
        outb(ATA_CMD, CMD_READ);

        if (wait_drq() < 0)
            return -1;

        for (int i = 0; i < 256; i++)
        {
            uint16_t w = inw(ATA_DATA);
            buf[s * 512 + i * 2] = (uint8_t)w;
            buf[s * 512 + i * 2 + 1] = (uint8_t)(w >> 8);
        }
    }
    return 0;
}

int ata_write(uint32_t lba, uint32_t count, const uint8_t *buf)
{
    if (!present || lba + count > sectors || lba >= 0x10000000)
        return -1;

    for (uint32_t s = 0; s < count; s++)
    {
        uint32_t l = lba + s;
        outb(ATA_DRIVE, 0xE0 | ((l >> 24) & 0x0F));
        outb(ATA_SECCOUNT, 1);
        outb(ATA_LBA_LO, (uint8_t)l);
        outb(ATA_LBA_MID, (uint8_t)(l >> 8));
        outb(ATA_LBA_HI, (uint8_t)(l >> 16));
        outb(ATA_CMD, CMD_WRITE);

        if (wait_drq() < 0)
            return -1;

        for (int i = 0; i < 256; i++)
        {
            uint16_t w = buf[s * 512 + i * 2] | ((uint16_t)buf[s * 512 + i * 2 + 1] << 8);
            outw(ATA_DATA, w);
        }
        outb(ATA_CMD, CMD_FLUSH);

        if (wait_ready() < 0)
            return -1;
    }
    return 0;
}