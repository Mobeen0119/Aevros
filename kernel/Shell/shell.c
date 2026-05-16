#include <stdint.h>
#include "../Include/shell.h"
#include "../Include/terminal.h"
#include "../Include/vfs.h"
#include "../LIB/string.c"
#include "../Inlcude/screen.h"



void shell_prompt(){
    kprint("FORGE_OS > ")
    
}
void shell_start(){
    char input[MAX_INPUT];

    while(1){
        shell_prompt();
        terminal_readline(input_line);
        shell_execute(input_line);
    }
}
