#include "../../Include/vfs.h"
#include "../../Include/screen.h"
#include "../../Lib/string.c"
#include "../Process/task.h"

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
    int fd = sys_open(".", READ_ONLY);

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

    while (n=(sys_read(fd,(uint8_t*) buff, sizeof(buff))) > 0)
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
    if (!path)
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

void cmd_write(const char *path, char *txt)
{
    if(!path || !txt){
        kprint("cat: missing args\n");
        return;
    }

    int fd = sys_open(path, WRITE_ONLY | CREAT);

    if (fd < 0)
    {
        kprint("\nWrite Failed\n");
        return;
    }

    sys_write(fd, (uint8_t *)txt, strlen(txt));
    sys_close(fd);
}

void cmd_rm(char *path)
{

    if (sys_unlink(path) == VFS_ERR)
    {
        kprint("rm: failed\n");
    }
}

void print_path(dentry_t* dir){
    if(!dir) return;

    if(dir->parent) print_path(dir->parent);

    kprint(
    "\n"
    );

    kprint(dir->name);
}
void cmd_pwd()
{ 
    print_path(current_task->cwd);
    kprint("\n");
    
}

void tree_walk(dentry_t *dir, int depth)
{
    if (!dir)
        return;

    for (int i = 0; i < depth; i++)
        kprint(" ");

    kprint(dir->name);
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
                kprint(child->name);
                kprint("  ");
            }
            child = child->hash_next;
        }
    }
}