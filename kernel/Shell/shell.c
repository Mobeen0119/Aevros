#include <stdint.h>
#include "../Include/shell.h"
#include "../Include/screen.h"
#include "../Include/terminal.h"
#include "../LIB/string.c"
#include "../Include/vfs.h"
#include "builtins.c"
#include "parser.c"

void shell_prompt()
{
    kprint("FORGE_OS > ");
}

void shell_execute(char *input)
{

    char *argv[MAX_ARG];
    int argc = tokenize(input, argv);

    if (argc == 0)
        return;

    if (strcmp(argv[0], "clear") == 0)
        kclear_screen();

    else if (strcmp(argv[0], "ls") == 0)
    {
        cmd_ls();
    }

    else if (strcmp(argv[0], "cat ") == 0)
    {
        if (argc < 2)
            kprint("cat: missing file\n");
        else
            cmd_cat(argv[1]);
    }

    else if (strcmp(argv[0], "echo ") == 0)
    {
        if (argc < 2)
            kprint("\n");
        else
        {
            for (int i = 1; i < argc; i++)
            {
                kprint(argv[i]);
                if (i != argc - 1)
                    kprint(" ");
            }
            kprint("\n");
        }
    }

    else if (strcmp(argv[0], "mkdir ") == 0)
    {
        sys_mkdir(argv[1]);
    }

    else if (strcmp(argv[0], "cd ") == 0)
    {
        if (argc < 2)
            kprint("cd: missing arg\n");
        else
            cmd_cd(argv[1]);
    }
  
    else if (strcmp(argv[0], "touch ") == 0)
    {
        if (argc < 2)
            kprint("touch: missing file\n");

        cmd_touch(argv[1]);
    }
  
    else if (strcmp(argv[0], "write ") == 0)
    {
        if (argc < 3)
        {
            kprint("Use : write <file> <text>\n");
        }
        cmd_write(argv[1], argv[2]);
    }
  
    else if (strcmp(argv[0], "rm") == 0)
    {
        if (argc < 2)
        {
            kprint("rm:failed\n");
        }
        cmd_rm(argv[1]);
    }
   
    else if (strcmp(argv[0], "pwd") == 0)
    {
        cmd_pwd();
    }
 
    else if (strcmp(argv[0], "tree") == 0)
    {
        tree_walk(vfs_read,0);
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
