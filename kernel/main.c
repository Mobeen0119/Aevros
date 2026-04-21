#include "screen.h"
#define RAM_SIZE 1024

struct PCB
{
    int pid;
    int ram;
    int state;
};

struct PCB process_list[3];
int used_ram=0;

void forge_init(){
    clear_screen();
    print("ForgeOS Kernel v1.0",0,25);
    print("Status: System Booted ",2,0);
}

void show_memory_status(){
    print("RAM Usage : ",5,0);
    print("Allocated 256MB to Process 1: ",7,0);
}


void main() {
    clear_screen();

    char* video_memory = (char*) 0xB8000;
    char* message = "Welcome to ForgeOS!";
    
    for (int i = 0; message[i] != '\0'; i++) {
        video_memory[i * 2] = message[i];
        video_memory[i * 2 + 1] = 0x0F; 
    }

    while(1); 
}

void __main() {}