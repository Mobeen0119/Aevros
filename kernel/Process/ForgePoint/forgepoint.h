#ifndef FORGEPOINT_H
#define FORGEPOINT_H

#include "../task.h"

int forgepoint_save(const char* name);
int forgepoint_restore(const char* name);
void forgepoint_list(void);

#endif