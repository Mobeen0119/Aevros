#include "../../Include/vfs.h"
#include "../../Include/screen.h"

void cmd_cd(char *path)
{
    if (!path)
    {
        kprint("cd: missing argument\n");
        return;
    }
    if (sys_chdir(path) == VFS_ERR)
    {
        kprint("No such Directory\n");
    }
}

void cmd_ls()
{
    int fd = sys_open('.', 0);
    if (fd < 0)
    {
        kprint("ls: failed");
        return;
    }

    dirent_t entry;

    while (sys_readdir(fd, &entry) > 0)
    {
        kprint(entry.name);
        kprint("  ");
    }
    kprint("\n");
    sys_close(fd);
}

void cmd_cat(char *path)
{
    int fd = sys_open(path, 0);
    if (fd < 0)
    {
        kprint("cat: failed");
        return;
    }
    char buff[128];
    int n;

    while (sys_read(fd, buff, sizeof(buff)) > 0)
    {
        for (int i = 0; i < n; i++)
        {
            kput_char(buff[i]);
        }
    }
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
    if (!pa)
    {
        kprint("touch: missing file");
        return;
    }
    int fd = sys_open(path, CREAT);

    if (fd < 0)
    {
        kprint("touch failed");
        return;
    }
    sys_close(fd);
}