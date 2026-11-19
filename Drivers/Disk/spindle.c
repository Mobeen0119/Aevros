#include "spindle.h"
#include "../../kernel/io.h"

#define SP_DATA 0x1F0
#define SP_SECCOUNT 0x1F2
#define SP_LBA_LO 0x1F3
#define SP_LBA_MID 0x1F4
#define SP_LBA_HI 0x1F5
#define SP_DRIVE 0x1F6
#define SP_STATUS 0x1F7

#define ST_ERR 0x01
#define ST_DRQ 0x08
#define ST_DF 0x20
#define ST_BSY 0x80

#define CMD_IDENTIFY 0xEC
#define CMD_READ 0x20
#define CMD_WRITE 0x30
#define CMD_FLUSH 0xE7

#define MAX_BURST 255
#define POLL_LIMIT 1000000

static uint32_t sectors;
static int present;

static int wait_idle(void)
{
    for (uint32_t i = 0; i < POLL_LIMIT; i++)
    {
        uint8_t s = inb(SP_STATUS);
        if (!(s & ST_BSY))
            return (s & (ST_ERR | ST_DF)) ? -1 : 0;
    }
    return -1;
}

static int wait_data(void)
{
    for (uint32_t i = 0; i < POLL_LIMIT; i++)
    {
        uint8_t s = inb(SP_STATUS);

        if (s & (ST_ERR | ST_DF))
            return -1;
        if (!(s & ST_BSY) && (s & ST_DRQ))

            return 0;
    }
    return -1;
}

static void select_lba(uint32_t lba, uint32_t count)
{
    outb(SP_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(SP_SECCOUNT, (uint8_t)count);

    outb(SP_LBA_LO, (uint8_t)lba);
    outb(SP_LBA_MID, (uint8_t)(lba >> 8));
    outb(SP_LBA_HI, (uint8_t)(lba >> 16));
}

int spindle_init(void)
{
    present = 0;
    sectors = 0;

    outb(SP_DRIVE, 0xA0);
    outb(SP_SECCOUNT, 0);
    outb(SP_LBA_LO, 0);
    outb(SP_LBA_MID, 0);
    outb(SP_LBA_HI, 0);
    outb(SP_STATUS, CMD_IDENTIFY);

    if (inb(SP_STATUS) == 0)
        return -1;
    if (wait_idle() < 0)
        return -1;
    if (inb(SP_LBA_MID) != 0 || inb(SP_LBA_HI) != 0)
        return -1;
    if (wait_data() < 0)
        return -1;

    uint16_t id[256];
    for (int i = 0; i < 256; i++)
        id[i] = inw(SP_DATA);

    sectors = (uint32_t)id[60] | ((uint32_t)id[61] << 16);
    present = sectors > 0;
    return present ? 0 : -1;
}

uint32_t spindle_sector_count(void)
{
    return present ? sectors : 0;
}

int spindle_read(uint32_t lba, uint32_t count, uint8_t *buf)
{
    if (!present || count == 0 || lba + count > sectors || lba >= 0x10000000)
        return -1;

    while (count > 0)
    {
        uint32_t burst = count < MAX_BURST ? count : MAX_BURST;
        select_lba(lba, burst);
        outb(SP_STATUS, CMD_READ);

        for (uint32_t s = 0; s < burst; s++)
        {
            if (wait_data() < 0)
                return -1;

            for (int i = 0; i < 256; i++)
            {
                uint16_t w = inw(SP_DATA);
                buf[i * 2] = (uint8_t)w;
                buf[i * 2 + 1] = (uint8_t)(w >> 8);
            }
            buf += 512;
        }
        lba += burst;
        count -= burst;
    }
    return 0;
}

int spindle_write(uint32_t lba, uint32_t count, const uint8_t *buf)
{
    if (!present || count == 0 || lba + count > sectors || lba >= 0x10000000)
        return -1;

    while (count > 0)
    {
        uint32_t burst = count < MAX_BURST ? count : MAX_BURST;
        select_lba(lba, burst);
        outb(SP_STATUS, CMD_WRITE);

        for (uint32_t s = 0; s < burst; s++)
        {
            if (wait_data() < 0)
                return -1;
            for (int i = 0; i < 256; i++)
            {
                uint16_t w = buf[i * 2] | ((uint16_t)buf[i * 2 + 1] << 8);
                outw(SP_DATA, w);
            }
            buf += 512;
        }
        if (wait_idle() < 0)
            return -1;

        lba += burst;
        count -= burst;
    }

    outb(SP_STATUS, CMD_FLUSH);
    return wait_idle();
}