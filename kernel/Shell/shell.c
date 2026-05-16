#include <stdint.h>
#include "../Include/shell.h"
#include "../Include/screen.h"
#include "../Include/terminal.h"
#include "../LIB/string.c"
#include "../Include/vfs.h"

void shell_prompt()
{
    kprint("FORGE_OS > ");
}

void shell_execute(char *cmd)
{
    if (strcmp(cmd, "clear") == 0)
        kclear_screen();

    else if (strcmp(cmd, "ls") == 0)
    {
        int fd = sys_open(".", 0);
        dirent_t d;

        while (sys_readdir(fd, &d) > 0)
        {
            kprint(d.name);
            kprint('\n');
        }
        sys_close(fd);
    }

    else if (strncmp(cmd, "cat ", 4) == 0)
    {
        char *path = cmd + 4;

        int fd = sys_open(path, 0);

        if (fd < 0)
        {
            kprint("File Not Found\n");
            return;
        }
        char buf[64];
        int n;

        while ((n = sys_read(fd, buf, 64)) > 0)
        {
            for (int i = 0; i < n; i++)
            {
                kput_char(buf[i]);
            }
        }
        sys_close(fd);
    }

    else if (strncmp(cmd, "echo ", 5) == 0)
    {
        kprint(cmd + 5);
        kprint("\n");
    }

    else if (strncmp(cmd, "mkdir ", 6) == 0)
    {
        sys_mkdir(cmd + 6);
    }

    else if (strncmp(cmd, "cd ", 3) == 0)
    {
        sys_chdir(cmd + 3);
        kprint("\n");
    }
    
    else
    {
        kprint("unknown command\n");
    }
}

void shell_start()
{
    char input[MAX_INPUT];

    while (1)
    {
        shell_prompt();
        terminal_readline(input);
        shell_execute(input);
    }
}
