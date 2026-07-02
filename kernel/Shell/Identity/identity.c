#include "identity.h"

#include "../../../Include/screen.h"
#include "../../../Include/terminal.h"
#include "../../../Lib/kprintf.h"

#include "../../Process/task.h"
#include "../../Memory/pmm.h"
#include "../MemInfo/mem_info.h"
#include "../../Process/TaskLife/tasklife.h"
#include "../builtins.h"
#include "../../Memory/KallocTracker/kalloc_tracker.h"
#include "../../../Drivers/keyboard.h"

extern uint32_t placement_address;
extern int next_pid;
extern task_t *current_task;

static void line(void)
{
    set_color(VGA_DARK_GREY,VGA_BLACK);

    for(int i=0;i<70;i++)
        kput_char('=');

    kput_char('\n');

    reset_color();
}

void identity_show(void)
{
    kclear_screen();

set_color(VGA_LIGHT_CYAN,VGA_BLACK);
kprint("==========================================================================\n");
kprint("                           NYROS 0.2 DEV\n");
kprint("==========================================================================\n");


    reset_color();

    set_color(VGA_WHITE,VGA_BLACK);
    kprint("             Experimental Operating System\n");
    kprint("                Build ---- Understand\n\n");

    set_color(VGA_YELLOW,VGA_BLACK);
    kprintf(" Version  : 0.2-dev");
    kprintf("            PID   : %u\n",current_task ? current_task->pid : 0);

    kprintf(" Tasks    : %d",next_pid-1);
    kprintf("            Ticks : %u\n\n",get_ticks());

    set_color(VGA_LIGHT_GREEN,VGA_BLACK);
    kprint("========================================================================\n");
    kprint("                         MAIN DASHBOARD\n");
    kprint("====================================================================================================================================================\n");

    reset_color();

    kprint("\n");

    set_color(VGA_BLACK,VGA_LIGHT_CYAN);
    kprint(" [1] MEMORY ");
    reset_color();

    kprint("   ");

    set_color(VGA_BLACK,VGA_LIGHT_GREEN);
    kprint(" [2] TASKS ");
    reset_color();

    kprint("   ");

    set_color(VGA_BLACK,VGA_LIGHT_MAGENTA);
    kprint(" [3] DEBUG ");
    reset_color();

    kprint("\n\n");

    set_color(VGA_BLACK,VGA_YELLOW);
    kprint(" [4] STORAGE ");
    reset_color();

    kprint("   ");

    set_color(VGA_BLACK,VGA_LIGHT_RED);
    kprint(" [5] DRIVERS ");
    reset_color();

    kprint("   ");

    set_color(VGA_BLACK,VGA_LIGHT_BLUE);
    kprint(" [6] ABOUT ");
    reset_color();

    kprint("\n\n");

    set_color(VGA_BLACK,VGA_DARK_GREY);
    kprint(" [0] EXIT ");
    reset_color();

    kprint("\n\n");

    set_color(VGA_DARK_GREY,VGA_BLACK);
    kprint("====================================================================================================================================================\n");
    kprint("Press a number to open a module.\n");
    kprint("ESC returns to the shell.\n");
    kprint("====================================================================================================================================================\n");

    reset_color();
}

void identity_command(void)
{
    while (1)
    {
        identity_show();

        char c = keyboard_getchar();

        switch(c)
        {
            case '1': meminfo_all(); break;
            case '2': tasklife_ps(); break;
            case '3': tracker_dump(); break;
            case '4': tree_walk(vfs_root,0); break;
            case '5': kprint("Driver Manager\n"); break;
            case '6':
                kprint("\n");
                kprint("NYROS is an experimental kernel built from scratch.\n");
                kprint("Everything from memory to scheduling is handcrafted.\n");
                break;
            case '0':
                return;
        }

        kprint("\nPress any key to return...");
        keyboard_getchar();
    }
}
