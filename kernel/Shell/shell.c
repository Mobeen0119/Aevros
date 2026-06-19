#include <stdint.h>
#include "../../Include/shell.h"
#include "../../Include/screen.h"
#include "../../Include/terminal.h"
#include "../../Lib/string.h"
#include "../../Include/vfs.h"
#include "builtins.h"
#include "parser.h"
#include "MemInfo/mem_info.h"
#include "../Process/TaskLife/tasklife.h"
#include "../Process/WhyAlive/whyalive.h"
#include "../Process/ForgePoint/forgepoint.h"
#include "../Process/StackMap/stackmap.h"
#include "../Memory/MemFreeze/memfreeze.h"

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

    else if (strcmp(argv[0], "cat") == 0)
    {
        if (argc < 2)
            kprint("cat: missing file\n");
        else
            cmd_cat(argv[1]);
    }

    else if (strcmp(argv[0], "echo") == 0)
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

    else if (strcmp(argv[0], "mkdir") == 0)
    {
        if (argc < 2)
            kprint("mkdir: missing directory name\n");
        else
            sys_mkdir(argv[1]);
    }

    else if (strcmp(argv[0], "cd") == 0)
    {
        if (argc < 2)
            kprint("cd: missing arg\n");
        else
            cmd_cd(argv[1]);
    }

    else if (strcmp(argv[0], "touch") == 0)
    {
        if (argc < 2)
        {
            kprint("touch: missing file\n");
            return;
        }

        cmd_touch(argv[1]);
    }

    else if (strcmp(argv[0], "write") == 0)
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
        if (vfs_root)
            tree_walk(vfs_root, 0);
        else
            kprint("tree: VFS root not initialized\n");
    }

    else if (strcmp(argv[0], "meminfo") == 0)
    {
        if (argc == 1)
            meminfo_all();
        else if (strcmp(argv[1], "pmm") == 0)
            meminfo_pmm();
        else if (strcmp(argv[1], "heap") == 0)
            meminfo_heap();
        else if (strcmp(argv[1], "paging") == 0)
            meminfo_paging();
        else if (strcmp(argv[1], "task") == 0)
            meminfo_task();
        else if (strcmp(argv[1], "buddy") == 0)
            meminfo_buddy();
        else if (strcmp(argv[1], "slab") == 0)
            meminfo_slab();
        else
            kprint("usage: meminfo [pmm|heap|paging|task|buddy|slab]\n");
    }

    else if (argc > 1 && strcmp(argv[1], "buddy") == 0)
    {
        meminfo_buddy();
    }

    else if (argc > 1 && strcmp(argv[1], "slab") == 0)
    {
        meminfo_slab();
    }

    else if (strcmp(argv[0], "memstory") == 0)
    {
        if (argc == 1)
            tracker_dump();

        else if (strcmp(argv[1], "ghosts") == 0)
            tracker_dump_ghosts();

        else if (strcmp(argv[1], "pid") == 0 && argc >= 3)
            tracker_dump_pid((uint32_t)katoi(argv[2]));

        else
            kprint("usage: memstory [ghosts | pid <n>]\n");
    }

    else if (strcmp(argv[0], "ps") == 0)
    {
        tasklife_ps();
    }

    else if (strcmp(argv[0], "tasklife") == 0)
    {
        if (argc < 2)
        {
            kprintf("usage: tasklife <pid> || <name>\n");
            return;
        }

        task_t *t = ready_queue;
        int found = 0;
        do
        {
            if (strcmp(t->name, argv[1]) == 0)
            {
                tasklife_dump(t->pid);
                found = 1;
                break;
            }
            t = t->next;
        } while (t != ready_queue);

        if (!found)
            tasklife_dump((uint32_t)katoi(argv[1]));
    }

    else if (strcmp(argv[0], "forgepoint") == 0)
    {
        if (argc < 3)
        {
            kprintf("usage: forgepoint <save|restore> <name>\n");
        }
        else if (strcmp(argv[1], "save") == 0)
            forgepoint_save(argv[2]);
        else if (strcmp(argv[1], "restore") == 0)
            forgepoint_restore(argv[2]);
        else if (strcmp(argv[1], "list") == 0)
            forgepoint_list();
        else
            kprintf("usage: forgepoint <save|restore> <name>\n");
    }

    else if (strcmp(argv[0], "whyalive") == 0)
    {
        if (argc < 3)
        {
            kprintf("usage: whyalive <inode|task|alloc> <path|pid|addr>\n");
        }
        else if (strcmp(argv[1], "inode") == 0)
            whyalive_inode_path(argv[2]);
        else if (strcmp(argv[1], "task") == 0)
            whyalive_task((uint32_t)katoi(argv[2]));
        else if (strcmp(argv[1], "alloc") == 0)
            whyalive_alloc((void *)(uintptr_t)parse_hex(argv[2]));
        else
            kprintf("usage: whyalive <inode|task|alloc> <path|pid|addr>\n");
    }

    else if (strcmp(argv[0], "stackmap") == 0)
    {
        if (argc < 2)
            kprintf("used: stackmap \n");
        else
            stackmap_dump(argv[1]);
    }

    else if (strcmp(argv[0], "memfreeze") == 0)
    {
        if (argc < 2)
            kprintf("usage: memfreeze <snap | diff>\n");
        else if (strcmp(argv[1], "snap") == 0)
            memfreeze_snap();
        else if (strcmp(argv[1], "diff") == 0)
            memfreeze_diff();
        else
            kprintf("usage: memfreeze <snap | diff>\n");
    }

    else if (strcmp(argv[0], "fdleak") == 0)
        fdleak_scan();
        
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
