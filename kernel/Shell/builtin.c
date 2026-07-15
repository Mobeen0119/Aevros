#include "../../Include/vfs.h"
#include "builtins.h"
#include "../../Include/screen.h"
#include "../../Lib/string.h"
#include "../Process/task.h"
#include "../../Lib/kprintf.h"
#include "../../Include/terminal.h"

void cmd_cd(char *path)
{
    if (!path)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprint("cd: missing argument\n");
        reset_color();
        return;
    }
    if (sys_chdir(path) == VFS_ERR)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("No such directory\n");
        reset_color();
    }
}

void cmd_ls()
{
    int fd = sys_open(".", READ_ONLY);

    if (fd < 0)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("ls: failed\n");
        reset_color();
        return;
    }

    dirent_t entry;
    int any = 0;

    while (sys_readdir(fd, &entry) > 0)
    {
        any = 1;
        if (entry.type & VFS_DIR)
            set_color(VGA_CYAN, VGA_BLACK);
        else
            set_color(VGA_WHITE, VGA_BLACK);
        kprint(entry.name);
        reset_color();
        kprint("  ");
    }

    if (!any)
    {
        set_color(VGA_DARK_GREY, VGA_BLACK);
        kprint("(empty)");
        reset_color();
    }

    kprint("\n");
    sys_close(fd);
}

void cmd_cat(char *path)
{
    int fd = sys_open(path, READ_ONLY);
    if (fd < 0)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("cat: failed\n");
        reset_color();
        return;
    }
    char buff[128];
    int n;

    set_color(VGA_WHITE, VGA_BLACK);
    while ((n = (sys_read(fd, (uint8_t *)buff, sizeof(buff)))) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            kput_char(buff[i]);
        }
    }
    reset_color();
    kprint("\n");
    sys_close(fd);
}

void cmd_echo(char *text)
{
    
    kprint(text);
    kprint("\n");
}

void cmd_touch(char *path)
{
    if (!path)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprint("touch: missing file\n");
        reset_color();
        return;
    }
    int fd = sys_open(path, CREAT);

    if (fd < 0)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("touch: failed\n");
        reset_color();
        return;
    }
    sys_close(fd);
}

void cmd_write(const char *path, char *txt)
{
    if (!path || !txt)
    {
        set_color(VGA_YELLOW, VGA_BLACK);
        kprint("write: missing args\n");
        reset_color();
        return;
    }

    int fd = sys_open(path, WRITE_ONLY | CREAT);

    if (fd < 0)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("write: failed\n");
        reset_color();
        return;
    }

    sys_write(fd, (uint8_t *)txt, strlen(txt));
    sys_close(fd);
}

void cmd_rm(char *path)
{

    if (sys_unlink(path) == VFS_ERR)
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("rm: failed\n");
        reset_color();
    }
}

void print_path(dentry_t *dir)
{
    if (!dir || dir == dir->parent)
        return;
    print_path(dir->parent);
    kprint("/");
    kprint(dir->name);
}

void cmd_pwd()
{
    if (current_task->cwd == vfs_root)
    {
        kprint("/\n");
        return;
    }
    print_path(current_task->cwd);
    kprint("\n");
}

void tree_walk(dentry_t *dir, int depth)
{
    if (!dir)
        return;

    for (int i = 0; i < depth; i++)
        kprint(" ");

    set_color(VGA_CYAN, VGA_BLACK);
    kprint(dir->name);
    reset_color();
    kprint("\n");

    for (int c = 0; c < DENTRY_HASH; c++)
    {
        dentry_t *child = dir->hash_bucket[c];

        while (child)
        {
            if (child->inode->flags & VFS_DIR)
                tree_walk(child, depth + 1);
            else
            {
                for (int m = 0; m < depth + 1; m++)
                    kprint("  ");
                set_color(VGA_WHITE, VGA_BLACK);
                kprint(child->name);
                reset_color();
                kprint("  ");
            }
            child = child->hash_next;
        }
    }
}