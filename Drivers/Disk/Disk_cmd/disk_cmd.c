#include "disk_cmd.h"
#include "../ata.h"
#include "../Craine/cairn.h"
#include "../../../Include/vfs.h"
#include "../../../kernel/Memory/kheap.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define DISK_BUF_MAX (128 * 1024)

static uint32_t fnv(const uint8_t *d, uint32_t n)
{
    uint32_t h = 2166136261u;

    for (uint32_t i = 0; i < n; i++)
        h = (h ^ d[i]) * 16777619u;
    return h;
}

static int ready_checked;
static int ready;

static int disk_ready(void)
{
    if (!ready_checked)
    {
        ready_checked = 1;
        ready = ata_init() == 0;
    }

    if (!ready)
        kprintf("disk: no disk on the primary channel (start QEMU with -hda disk.img)\n");
    return ready;
}

void disk_command(int argc, char **argv)
{
    if (argc < 2)
    {
        kprintf("usage: disk info | list | save <name> <ramfs file> | load <name> <ramfs file>\n");
        return;
    }

    if (strcmp(argv[1], "info") == 0)
    {
        if (!disk_ready())
            return;

        uint32_t s = ata_sector_count();
        kprintf("disk: %u sectors (%u MB) on the primary channel\n", s, s / 2048);
        return;
    }

    if (strcmp(argv[1], "list") == 0)
    {
        if (!disk_ready())
            return;

        cairn_list();
        return;
    }

    if ((strcmp(argv[1], "save") == 0 || strcmp(argv[1], "load") == 0) && argc >= 4)
    {
        if (!disk_ready())
            return;

        uint8_t *buf = kmalloc_raw(DISK_BUF_MAX);
        if (!buf)
        {
            kprintf("disk: out of memory\n");
            return;
        }

        if (strcmp(argv[1], "save") == 0)
        {
            int fd = sys_open(argv[3], READ_ONLY);

            if (fd < 0)
            {
                kprintf("disk: no file %s in RAMFS\n", argv[3]);
                kfree_raw(buf);
                return;
            }
            uint32_t total = 0;

            for (;;)
            {
                if (total == DISK_BUF_MAX)
                    break;

                int n = sys_read(fd, buf + total, DISK_BUF_MAX - total);

                if (n <= 0)
                    break;
                total += (uint32_t)n;
            }
            sys_close(fd);
            int r = cairn_save(argv[2], buf, total);

            if (r == 0)
                kprintf("disk: saved %u bytes as %s, fnv1a %x\n", total, argv[2], fnv(buf, total));
            else
                kprintf("disk: save failed (%d)\n", r);
        }
        else
        {
            uint32_t size = 0;
            int r = cairn_load(argv[2], buf, DISK_BUF_MAX, &size);

            if (r == 0)
            {
                int fd = sys_open(argv[3], READ_WRITE | CREAT);

                if (fd < 0)
                    kprintf("disk: cannot create %s\n", argv[3]);

                else
                {
                    sys_write(fd, buf, size);
                    sys_close(fd);
                    kprintf("disk: loaded %s (%u bytes, fnv1a %x) into %s\n", argv[2], size, fnv(buf, size), argv[3]);
                }
            }
            else
                kprintf("disk: load failed (%d)\n", r);
        }

        kfree_raw(buf);
        return;
    }

    kprintf("usage: disk info | list | save <name> <ramfs file> | load <name> <ramfs file>\n");
}