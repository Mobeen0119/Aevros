
#include "../Process/task.c"
#include "../isr.h"


void sysCall_handler(struct registers *r){
    switch (r->eax)
    {
    case 1:
      sys_print((char*) r->eax);
        break;
    case 2:
      sys_exit();
      break;
    default:
        break;
    }
}



