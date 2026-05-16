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
