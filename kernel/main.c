#include <stdio.h>
#include <stdlib.h>
#define RAM_SIZE 1024

int available_ram = RAM_SIZE;

typedef struct
{
    int pid;
    int burst_time;
    int ram;
    int state;
} Process;

typedef struct
{
    int processes[3];
    int count;
} PCB;

void add_process(PCB *pcb, Process *p)
{
    if (pcb->count >= 3)
    {
        printf("Error: PCB is full!\n");
        return;
    }
    if (p->ram < RAM_SIZE && p->ram <= available_ram)
    {
        pcb->processes[pcb->count] = p->pid;
        pcb->count++;
        available_ram -= p->ram;
        printf("Process %d added to PCB.\n", p->pid);
    }
    else
    {
        printf("Error: Not enough RAM for Process %d!\n", p->pid);
    }
}

void dellocate(PCB *pcb, Process *p)
{
    for (int i = 0; i < pcb->count; i++)
    {
        if (pcb->processes[i] == p->pid)
        {
            for (int j = i; j < pcb->count - 1; j++)
            {
                pcb->processes[j] = pcb->processes[j + 1];
            }
            pcb->count--;
            available_ram += p->ram;
            printf("Process %d deallocated from PCB.\n", p->pid);
            return;
        }
    }
    printf("Error: Process %d not found in PCB!\n", p->pid);
}
int main()
{
    PCB myPCB = {.count = 0};

    Process p1 = {.pid = 101, .burst_time = 5, .ram = 250};

    Process p2 = {.pid = 102, .burst_time = 10, .ram = 900};

    add_process(&myPCB, &p1);
     dellocate(&myPCB, &p1);
    add_process(&myPCB, &p2);

    return 0;
}