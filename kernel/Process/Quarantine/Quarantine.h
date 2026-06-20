#ifndef QUARANTINE_H
#define QUARANTINE_H

#include "../task.h"
#include <stdint.h>

void quarantine_check_and_act(void);   
void quarantine_list(void);
void quarantine_release(uint32_t pid);

#endif