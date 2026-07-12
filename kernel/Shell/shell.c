#include <stdint.h>
#include "../../Include/shell.h"
#include "../../Include/screen.h"
#include "../../Include/terminal.h"
#include "../../Lib/string.h"
#include "../../Include/vfs.h"
#include "builtins.h"
#include "parser.h"
#include "MemInfo/mem_info.h"
#include "../../Lib/kprintf.h"
#include "../Process/TaskLife/tasklife.h"
#include "../Process/WhyAlive/whyalive.h"
#include "../Process/AevrosPoint/aevrospoint.h"
#include "../Process/StackMap/stackmap.h"
#include "../Memory/MemFreeze/memfreeze.h"
#include "../Process/OutLook/outlook.h"
#include "../Process/Quarantine/Quarantine.h"
#include "../Process/Blast/blast.h"
#include "../Process/TIMELINE/timeline.h"
#include "../Memory/KallocTracker/kalloc_tracker.h"
#include "../Process/FDLeak/fdleak.h"
#include "../Memory/buddy.h"
#include "../selftest.h"
#include "../Process/exec.h"
#include "Identity/identity.h"

void shell_prompt(void)
{
    set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    kprint("Aevros");
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprint(" > ");
    set_color(VGA_WHITE, VGA_BLACK);
}

static void help_section(const char *title)
{
    set_color(VGA_YELLOW, VGA_BLACK);
    kprintf("  %s\n", title);
    reset_color();
}

static void help_line(const char *cmd, const char *desc)
{
    set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    kprintf("    %-30s", cmd);
    set_color(VGA_LIGHT_GREY, VGA_BLACK);
    kprintf("%s\n", desc);
    reset_color();
}

void shell_execute(char *input)
{

    char *argv[MAX_ARG];
    int argc = tokenize(input, argv);


    if (argc == 0)
        return;

    if (strcmp(argv[0], "clear") == 0)
        kclear_screen();

    if (strcmp(argv[0], "checktest") == 0)
    {
        aevrospoint_full_test();
    }

    else if (strcmp(argv[0], "fork") == 0)
    {
        int pid = sys_exec("/forktest.elf");
        if (pid < 0)
        {
            set_color(VGA_RED, VGA_BLACK);
            kprint("fork: failed to run /forktest.elf\n");
            reset_color();
        }
        else
        {
            int status = 0;
            asm volatile("sti");
            sys_waitpid(pid, &status);
            set_color(status == 0 ? VGA_GREEN : VGA_RED, VGA_BLACK);
            kprintf("fork: test exited with code %d\n", status);
            reset_color();
        }
    }

    else if (strcmp(argv[0], "exec") == 0)
    {
        const char *path = (argc < 2) ? "/exectest.elf" : argv[1];
        int pid = sys_exec(path);
        if (pid < 0)
        {
            set_color(VGA_RED, VGA_BLACK);
            kprintf("exec: failed to run %s\n", path);
            reset_color();
        }
        else
        {
            int status = 0;
            asm volatile("sti");
            sys_waitpid(pid, &status);
            set_color(status == 0 ? VGA_GREEN : VGA_RED, VGA_BLACK);
            kprintf("exec: %s exited with code %d\n", path, status);
            reset_color();
        }
    }

    else if (strcmp(argv[0], "identity") == 0)
    {
        identity_command();
    }

    else if (strcmp(argv[0], "ls") == 0)
    {
        cmd_ls();
    }

    else if (strcmp(argv[0], "cat") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("cat: missing file\n");
            reset_color();
        }
        else
            cmd_cat(argv[1]);
    }

    else if (strcmp(argv[0], "echo") == 0)
    {
        if (argc < 2)
        {
            kprint("\n");
        }
        else
        {
            int redirect_at = -1;
            for (int i = 1; i < argc; i++)
            {
                if (strcmp(argv[i], ">") == 0)
                {
                    redirect_at = i;
                    break;
                }
            }

            if (redirect_at >= 0 && redirect_at + 1 < argc)
            {
                static char text_buf[512];
                int pos = 0;
                for (int i = 1; i < redirect_at && pos < (int)sizeof(text_buf) - 1; i++)
                {
                    int j = 0;
                    while (argv[i][j] && pos < (int)sizeof(text_buf) - 1)
                        text_buf[pos++] = argv[i][j++];
                    if (i != redirect_at - 1 && pos < (int)sizeof(text_buf) - 1)
                        text_buf[pos++] = ' ';
                }
                text_buf[pos] = '\0';
                cmd_write(argv[redirect_at + 1], text_buf);
            }
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
    }

    else if (strcmp(argv[0], "mkdir") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("mkdir: missing directory name\n");
            reset_color();
        }
        else
            sys_mkdir(argv[1]);
    }

    else if (strcmp(argv[0], "cd") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("cd: missing arg\n");
            reset_color();
        }
        else
            cmd_cd(argv[1]);
    }

    else if (strcmp(argv[0], "touch") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("touch: missing file\n");
            reset_color();
            return;
        }

        cmd_touch(argv[1]);
    }

    else if (strcmp(argv[0], "write") == 0)
    {
        if (argc < 3)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("Use : write <file> <text>\n");
            reset_color();
            return;
        }

        static char text_buf[512];
        int pos = 0;
        for (int i = 2; i < argc && pos < (int)sizeof(text_buf) - 1; i++)
        {
            int j = 0;
            while (argv[i][j] && pos < (int)sizeof(text_buf) - 1)
                text_buf[pos++] = argv[i][j++];
            if (i != argc - 1 && pos < (int)sizeof(text_buf) - 1)
                text_buf[pos++] = ' ';
        }
        text_buf[pos] = '\0';

        cmd_write(argv[1], text_buf);
    }

    else if (strcmp(argv[0], "rm") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_RED, VGA_BLACK);
            kprint("rm:failed\n");
            reset_color();
            return;
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
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("tree: VFS root not initialized\n");
            reset_color();
        }
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
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("usage: meminfo [pmm|heap|paging|task|buddy|slab]\n");
            reset_color();
        }
    }

    else if (strcmp(argv[0], "memstory") == 0)
    {
        if (argc == 1)
            tracker_dump();

        else if (strcmp(argv[1], "ghosts") == 0)
            tracker_dump_ghosts();

        else if (strcmp(argv[1], "pid") == 0 && argc == 3)
            tracker_dump_pid((uint32_t)katoi(argv[2]));

        else
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprint("usage: memstory [ghosts | pid <n>]\n");
            reset_color();
        }
    }

    else if (strcmp(argv[0], "ps") == 0)
    {
        tasklife_ps();
    }

    else if (strcmp(argv[0], "tasklife") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: tasklife <pid> || <name>\n");
            reset_color();
            return;
        }

        if (!ready_queue)
        {
            kprintf("tasklife: no tasks running\n");
            return;
        }

        asm volatile("cli");
        task_t *t = ready_queue;
        int found = 0;
        uint32_t found_pid = 0;
        do
        {
            if (strcmp(t->name, argv[1]) == 0)
            {
                found_pid = t->pid;
                found = 1;
                break;
            }
            t = t->next;
        } while (t != ready_queue);
        asm volatile("sti");

        if (found)
            tasklife_dump(found_pid);
        else
            tasklife_dump((uint32_t)katoi(argv[1]));
    }

    else if (strcmp(argv[0], "checkpoint") == 0)
    {
        if (argc >= 2 && strcmp(argv[1], "list") == 0)
        {
            aevrospoint_list();
        }
        else if (argc < 3)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: checkpoint <save|restore> <name>  or  checkpoint list\n");
            reset_color();
        }
        else if (strcmp(argv[1], "save") == 0)
        {
            aevrospoint_save(argv[2]);
        }
        else if (strcmp(argv[1], "restore") == 0)
        {
            aevrospoint_restore(argv[2]);
        }
        else
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: checkpoint <save|restore> <name>  or  checkpoint list\n");
            reset_color();
        }
    }

    else if (strcmp(argv[0], "whyalive") == 0)
    {
        if (argc < 3)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: whyalive <inode|task|alloc> <path|pid|addr>\n");
            reset_color();
        }
        else if (strcmp(argv[1], "inode") == 0)
            whyalive_inode_path(argv[2]);
        else if (strcmp(argv[1], "task") == 0)
            whyalive_task((uint32_t)katoi(argv[2]));
        else if (strcmp(argv[1], "alloc") == 0)
            whyalive_alloc((void *)(uintptr_t)parse_hex(argv[2]));
        else
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: whyalive <inode|task|alloc> <path|pid|addr>\n");
            reset_color();
        }
    }

    else if (strcmp(argv[0], "buddydbg") == 0)
    {
        for (int o = 0; o <= MAX_ORDER; o++)
            buddy_debug_list(o);
    }

    else if (strcmp(argv[0], "stackmap") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("used: stackmap \n");
            reset_color();
        }
        else
            stackmap_dump(argv[1]);
    }

    else if (strcmp(argv[0], "ticks") == 0)
    {
        kprintf("ticks=%u\n", get_ticks());
    }

    else if (strcmp(argv[0], "memfreeze") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: memfreeze <snap | diff>\n");
            reset_color();
        }
        else if (strcmp(argv[1], "snap") == 0)
            memfreeze_snap();
        else if (strcmp(argv[1], "diff") == 0)
            memfreeze_diff();
        else
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: memfreeze <snap | diff>\n");
            reset_color();
        }
    }

    else if (strcmp(argv[0], "fdleak") == 0)
        fdleak_scan();

    else if (strcmp(argv[0], "outlook") == 0)
        outlook_scan();

    else if (strcmp(argv[0], "timeline") == 0)
        timeline_dump();

    else if (strcmp(argv[0], "quarantine") == 0)
    {
        if (argc == 1)
            quarantine_list();

        else if (strcmp(argv[1], "check") == 0)
            quarantine_check_and_act();

        else if (strcmp(argv[1], "release") == 0 && argc >= 3)
            quarantine_release(argv[2]);

        else
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: quarantine [check|release <name>]\n");
            reset_color();
        }
    }

    else if (strcmp(argv[0], "blast") == 0)
    {
        if (argc < 2)
        {
            set_color(VGA_YELLOW, VGA_BLACK);
            kprintf("usage: blast <pid>\n");
            reset_color();
        }
        else
            blast_radius((uint32_t)katoi(argv[1]));
    }

    else if (strcmp(argv[0], "selftest") == 0)
        selftest_run();

    else if (strcmp(argv[0], "help") == 0)
    {
        set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        kprint("\n  Aevros shell commands\n\n");
        reset_color();

        help_section("general");
        help_line("clear", "clear the screen");
        help_line("identity", "system dashboard");
        help_line("help", "show this list");
        kprint("\n");

        help_section("filesystem");
        help_line("ls", "list directory contents");
        help_line("pwd", "print working directory");
        help_line("cd <dir>", "change directory");
        help_line("mkdir <dir>", "create a directory");
        help_line("touch <file>", "create an empty file");
        help_line("write <file> <text>", "write text to a file");
        help_line("cat <file>", "print file contents");
        help_line("echo <text>", "print text");
        help_line("echo <text> > <file>", "write text to a file");
        help_line("rm <file>", "remove a file");
        help_line("tree", "show the directory tree");
        kprint("\n");

        help_section("processes");
        help_line("exec <file>", "run a program");
        help_line("fork", "run the fork demo program");
        help_line("ps", "list tasks");
        help_line("ticks", "show system tick count");
        help_line("tasklife <pid|name>", "task lifetime info");
        help_line("checkpoint <save|restore|list> <name>", "save/restore a process");
        kprint("\n");

        help_section("memory & diagnostics");
        help_line("meminfo [subsystem]", "memory subsystem info");
        help_line("memstory [ghosts|pid <n>]", "allocation tracker");
        help_line("memfreeze <snap|diff>", "memory snapshot/diff");
        help_line("buddydbg", "buddy allocator debug view");
        help_line("stackmap <pid>", "stack usage map");
        help_line("whyalive <inode|task|alloc>", "liveness inspector");
        help_line("fdleak", "file descriptor leak check");
        help_line("outlook", "process outlook view");
        help_line("timeline", "event timeline");
        help_line("quarantine [check|release]", "quarantine control");
        help_line("blast <pid>", "blast radius report");
        kprint("\n");

        help_section("testing");
        help_line("selftest", "run built-in kernel tests");
    }

    else
    {
        set_color(VGA_RED, VGA_BLACK);
        kprint("unknown command\n");
        reset_color();
    }
}

void shell_start(void)
{
    char input[MAX_INPUT];

    while (1)
    {
        shell_prompt();
        terminal_readline(input);
        shell_execute(input);

        if (current_task->is_checkpoint_clone)
        {
            kprintf("\n[checkpoint clone PID %d finished its resumed command, going idle -- keyboard/screen stay owned by the original shell]\n", current_task->pid);
            while (1)
                schedule();
        }
    }
}