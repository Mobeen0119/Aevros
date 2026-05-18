#include <stdint.h>
#include "../Include/shell.h"
#include "../Include/screen.h"
#include "../Include/terminal.h"
#include "../LIB/string.c"
#include "../Include/vfs.h"
#include "builtins.c"

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
        cmd_ls();
    }

    else if (strncmp(cmd, "cat ", 4) == 0)
    {
        cmd_cat(cmd + 4);
    }

    else if (strncmp(cmd, "echo ", 5) == 0)
    {
        cmd_echo();
    }

    else if (strncmp(cmd, "mkdir ", 6) == 0)
    {
        sys_mkdir(cmd + 6);
    }

    else if (strncmp(cmd, "cd ", 3) == 0)
    {

        cmd_cd(cmd + 3);
    }

    else
    {
        kprint("unknown command\n");
    }
}

void shell_start()
{
    char input_l[MAX_INPUT];

    while (1)
    {
        shell_prompt();
        terminal_readline(input_l);
        shell_execute(input_l);
    }
}
