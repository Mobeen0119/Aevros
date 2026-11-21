#include "../Shelf_cmd/shelf_cmd.h"
#include "../spindle.h"
#include "../Shelf/shelf.h"
#include "../../../Include/vfs.h"
#include "../../../kernel/Memory/kheap.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"

#define SHELF_BUF_MAX (128 * 1024)

static uint32_t checksum(const uint8_t *d, uint32_t n)
{
    uint32_t h = 2166136261u;
    for (uint32_t i = 0; i < n; i++)
        h = (h ^ d[i]) * 16777619u;
    return h;
}

static int spindle_ready;
static int spindle_checked;

static int disk_ready(void)
{
    if (!spindle_checked)
    {
        spindle_checked = 1;
        spindle_ready = spindle_init() == 0;
    }
    if (!spindle_ready)
        kprintf("shelf: no disk on the primary channel (start QEMU with -hda disk.img)\n");
    return spindle_ready;
}

static void usage(void)
{
    kprintf("usage: shelf info | list | save <name> <ramfs file> | load <name> <ramfs file>\n");
}

void shelf_command(int argc, char **argv)
{
    if (argc < 2)
    {
        usage();
        return;
    }

    if (strcmp(argv[1], "info") == 0)
    {
        if (!disk_ready())
            return;
        uint32_t s = spindle_sector_count();
        kprintf("shelf: disk has %u sectors (%u MB)\n", s, s / 2048);
        return;
    }

    if (strcmp(argv[1], "list") == 0)
    {
        if (!disk_ready())
            return;
        shelf_list();
        return;
    }

    if ((strcmp(argv[1], "save") == 0 || strcmp(argv[1], "load") == 0) && argc >= 4)
    {
        if (!disk_ready())
            return;

        uint8_t *buf = kmalloc_raw(SHELF_BUF_MAX);
        if (!buf)
        {
            kprintf("shelf: out of memory\n");
            return;
        }

        if (strcmp(argv[1], "save") == 0)
        {
            int fd = sys_open(argv[3], READ_ONLY);
            if (fd < 0)
            {
                kprintf("shelf: no file %s in RAMFS\n", argv[3]);
                kfree_raw(buf);
                return;
            }
            uint32_t total = 0;
            while (total < SHELF_BUF_MAX)
            {
                int n = sys_read(fd, buf + total, SHELF_BUF_MAX - total);
                if (n <= 0)
                    break;
                total += (uint32_t)n;
            }
            sys_close(fd);

            int r = shelf_save(argv[2], buf, total, SHELF_BUF_MAX);
            if (r == 0)
                kprintf("shelf: saved %u bytes as %s, fnv1a %x\n", total, argv[2], checksum(buf, total));
            else
                kprintf("shelf: save failed (%d)\n", r);
        }
        else
        {
            uint32_t size = 0;
            int r = shelf_load(argv[2], buf, SHELF_BUF_MAX, &size);
            if (r == 0)
            {
                int fd = sys_open(argv[3], READ_WRITE | CREAT);
                if (fd < 0)
                    kprintf("shelf: cannot create %s\n", argv[3]);
                else
                {
                    sys_write(fd, buf, size);
                    sys_close(fd);
                    kprintf("shelf: loaded %s (%u bytes, fnv1a %x) into %s\n", argv[2], size, checksum(buf, size), argv[3]);
                }
            }
            else
                kprintf("shelf: load failed (%d)\n", r);
        }

        kfree_raw(buf);
        return;
    }

    usage();
}