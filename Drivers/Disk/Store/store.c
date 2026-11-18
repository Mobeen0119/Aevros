#include "store.h"
#include "ata.h"
#include "../../../Lib/string.h"
#include "../../../Lib/kprintf.h"

#define STORE_DIR_LBA 2048
#define STORE_FIRST_DATA 2049
#define ENTRY_COUNT 16
#define NAME_MAX 23

typedef struct
{
    char name[24];
    uint32_t lba;
    uint32_t size;

} entry_t;

static int read_dir(entry_t *dir)
{
    static uint8_t sector[512];

    if (ata_read(STORE_DIR_LBA, 1, sector) < 0)
        return -1;

    memcpy(dir, sector, sizeof(entry_t) * ENTRY_COUNT);
    return 0;
}

static int write_dir(const entry_t *dir)
{
    static uint8_t sector[512];
    memset(sector, 0, sizeof(sector));

    memcpy(sector, dir, sizeof(entry_t) * ENTRY_COUNT);
    return ata_write(STORE_DIR_LBA, 1, sector);
}

static uint32_t next_free(const entry_t *dir)
{
    uint32_t next = STORE_FIRST_DATA;

    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (dir[i].name[0] == 0)
            continue;
        uint32_t end = dir[i].lba + (dir[i].size + 511) / 512;

        if (end > next)
            next = end;
    }
    return next;
}

int store_save(const char *name, const uint8_t *data, uint32_t size)
{
    if (!name || !name[0] || strlen(name) > NAME_MAX)
        return -1;

    if (ata_sector_count() < STORE_FIRST_DATA)
        return -1;

    entry_t dir[ENTRY_COUNT];

    if (read_dir(dir) < 0)
        return -1;

    int slot = -1;

    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (dir[i].name[0] == 0 && slot < 0)
            slot = i;

        if (dir[i].name[0] && strcmp(dir[i].name, name) == 0)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return -2;

    uint32_t lba = next_free(dir);
    uint32_t sectors = (size + 511) / 512;

    if (lba + sectors > ata_sector_count())
        return -3;

    if (sectors > 0)
    {
        static uint8_t pad[512];

        for (uint32_t s = 0; s < sectors; s++)
        {
            uint32_t n = size - s * 512 < 512 ? size - s * 512 : 512;
            memset(pad, 0, 512);

            memcpy(pad, data + s * 512, n);
            if (ata_write(lba + s, 1, pad) < 0)
                return -4;
        }
    }

    memset(dir[slot].name, 0, sizeof(dir[slot].name));
    memcpy(dir[slot].name, name, strlen(name));
    dir[slot].lba = lba;
    dir[slot].size = size;

    if (write_dir(dir) < 0)
        return -4;
    return 0;
}

int store_load(const char *name, uint8_t *out, uint32_t max, uint32_t *size_out)
{
    entry_t dir[ENTRY_COUNT];
    if (read_dir(dir) < 0)
        return -1;

    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (dir[i].name[0] == 0 || strcmp(dir[i].name, name) != 0)
            continue;

        if (dir[i].size > max)
            return -2;

        uint32_t sectors = (dir[i].size + 511) / 512;
        static uint8_t sector[512];

        for (uint32_t s = 0; s < sectors; s++)
        {
            if (ata_read(dir[i].lba + s, 1, sector) < 0)
                return -3;
            uint32_t n = dir[i].size - s * 512 < 512 ? dir[i].size - s * 512 : 512;
            memcpy(out + s * 512, sector, n);
        }
        *size_out = dir[i].size;
        return 0;
    }
    return -4;
}

void store_list(void)
{
    entry_t dir[ENTRY_COUNT];
    if (read_dir(dir) < 0)
    {
        kprintf("disk: cannot read the store directory\n");
        return;
    }
    int any = 0;
    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (dir[i].name[0] == 0)
            continue;
        any = 1;
        kprintf("  %s  %u bytes\n", dir[i].name, dir[i].size);
    }
    if (!any)
        kprintf("  (no files stored on disk)\n");
}