#include "shelf.h"
#include "../spindle.h"
#include "../../../Lib/string.h"
#include "../../../Lib/kprintf.h"

#define SHELF_DIR_LBA 2048
#define SHELF_FIRST_DATA 2049
#define ENTRY_COUNT 16
#define NAME_MAX 19
#define SECTOR 512

typedef struct
{
    char name[20];
    uint32_t lba;
    uint32_t size;
    uint32_t alloc;
} entry_t;

static int read_dir(entry_t *dir)
{
    static uint8_t sector[SECTOR];

    if (spindle_read(SHELF_DIR_LBA, 1, sector) < 0)
        return -1;

    memcpy(dir, sector, sizeof(entry_t) * ENTRY_COUNT);
    return 0;
}

static int write_dir(const entry_t *dir)
{
    static uint8_t sector[SECTOR];
    memset(sector, 0, sizeof(sector));
    memcpy(sector, dir, sizeof(entry_t) * ENTRY_COUNT);

    return spindle_write(SHELF_DIR_LBA, 1, sector);
}

static uint32_t sectors_for(uint32_t bytes)
{
    return (bytes + SECTOR - 1) / SECTOR;
}

static uint32_t next_free(const entry_t *dir, int skip)
{
    uint32_t next = SHELF_FIRST_DATA;

    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (i == skip || dir[i].name[0] == 0)
            continue;
        uint32_t end = dir[i].lba + dir[i].alloc;

        if (end > next)
            next = end;
    }
    return next;
}

int shelf_save(const char *name, uint8_t *data, uint32_t size, uint32_t capacity)
{
    if (!name || !name[0] || strlen(name) > NAME_MAX)
        return -1;
    uint32_t need = sectors_for(size);

    if (need * SECTOR > capacity)
        return -2;

    if (spindle_sector_count() < SHELF_FIRST_DATA)
        return -3;

    entry_t dir[ENTRY_COUNT];
    if (read_dir(dir) < 0)
        return -4;

    int slot = -1;
    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (dir[i].name[0] && strcmp(dir[i].name, name) == 0)
        {
            slot = i;
            break;
        }
        if (dir[i].name[0] == 0 && slot < 0)
            slot = i;
    }

    if (slot < 0)
        return -5;

    uint32_t lba;
    uint32_t alloc;
    if (dir[slot].name[0] && need <= dir[slot].alloc)
    {
        lba = dir[slot].lba;
        alloc = dir[slot].alloc;
    }
    else
    {
        lba = next_free(dir, slot);
        alloc = need;
        if (lba + alloc > spindle_sector_count())
            return -6;
    }

    if (need > 0)
    {
        memset(data + size, 0, need * SECTOR - size);
        if (spindle_write(lba, need, data) < 0)
            return -7;
    }

    memset(dir[slot].name, 0, sizeof(dir[slot].name));
    memcpy(dir[slot].name, name, strlen(name));
    dir[slot].lba = lba;
    dir[slot].size = size;
    dir[slot].alloc = alloc;

    if (write_dir(dir) < 0)
        return -8;

    return 0;
}

int shelf_load(const char *name, uint8_t *out, uint32_t capacity, uint32_t *size_out)
{
    entry_t dir[ENTRY_COUNT];

    if (read_dir(dir) < 0)
        return -1;

    for (int i = 0; i < ENTRY_COUNT; i++)
    {
        if (dir[i].name[0] == 0 || strcmp(dir[i].name, name) != 0)
            continue;

        uint32_t need = sectors_for(dir[i].size);
        if (need * SECTOR > capacity)
            return -2;
        if (need > 0 && spindle_read(dir[i].lba, need, out) < 0)
            return -3;

        *size_out = dir[i].size;
        return 0;
    }
    return -4;
}

void shelf_list(void)
{
    entry_t dir[ENTRY_COUNT];

    if (read_dir(dir) < 0)
    {
        kprintf("shelf: cannot read the directory\n");
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
        kprintf("  (nothing stored yet)\n");
}