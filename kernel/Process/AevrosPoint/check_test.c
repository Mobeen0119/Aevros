#include "aevrospoint.h"
#include "../../../Include/vfs.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
#include "../../../Include/terminal.h"
#include "../../Memory/kheap.h"
#include "../task.h"

static void build_path(const char *name, char *out, uint32_t outlen) {
    strncpy(out, "/checkpoint_", outlen);
    uint32_t base_len = strlen(out);
    strncpy(out + base_len, name, outlen - base_len - 5);
    strncpy(out + strlen(out), ".ckpt", outlen - strlen(out));
}

static void remove_extension(char *name) {
    int i = 0;
    while (name[i] && name[i] != '.') {
        i++;
    }
    if (name[i] == '.') {
        name[i] = '\0';
    }
}

int aevrospoint_test_save(void) {
    kprintf("\n=== Testing Checkpoint SAVE ===\n");
    
    kprintf("Saving current task 'shell'...\n");
    int result = aevrospoint_save("shell");
    if (result == VFS_OK) {
        kprintf("Save SUCCESSFUL\n");
    } else {
        kprintf("Save FAILED (error: %d)\n", result);
    }
    
    aevrospoint_list();
    kprintf("=== Save Test Complete ===\n\n");
    return result;
}

void aevrospoint_test_restore(void) {
    kprintf("\n=== Testing Checkpoint RESTORE ===\n");
    
    kprintf("Restoring 'shell' from checkpoint...\n");
    int pid = aevrospoint_restore("shell");
    if (pid > 0) {
        kprintf("Restore SUCCESSFUL! New PID: %d\n", pid);
    } else {
        set_color(VGA_RED,VGA_BLACK);
        kprintf("                          Restore FAILED (error: %d)\n", pid);
        reset_color();
    }
    
    kprintf("=== Restore Test Complete ===\n\n");
}

void aevrospoint_full_test(void) {
    kprintf("\n========================================\n");
    kprintf("        CHECKPOINT SYSTEM TEST            \n");
    kprintf("========================================\n");
    
    int save_result = aevrospoint_test_save();

    if (current_task->is_checkpoint_clone) {
        kprintf("This task is a resumed checkpoint clone (PID %d).\n", current_task->pid);
        kprintf("Stopping here instead of restoring again, to avoid an endless chain.\n");
        kprintf("========================================\n");
        kprintf("     TEST COMPLETE (resumed clone)     \n");
        kprintf("========================================\n\n");
        return;
    }

    if (save_result != VFS_OK) {
        kprintf("Skipping the restore test since the save step didn't succeed.\n");
        kprintf("========================================\n");
        kprintf("     TEST COMPLETE (save failed)       \n");
        kprintf("========================================\n\n");
        return;
    }

    kprintf("Press any key to continue to restore test...\n");
    
    aevrospoint_test_restore();
    
    kprintf("========================================\n");
    kprintf("     TEST COMPLETE                     \n");
    kprintf("========================================\n\n");
}
