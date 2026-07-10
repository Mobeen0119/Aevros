#ifndef AEVROSPOINT_H
#define AEVROSPOINT_H

#include "../task.h"

int aevrospoint_save(const char* name);
int aevrospoint_restore(const char* name);
void aevrospoint_list(void);

void aevrospoint_test(void);
void aevrospoint_stress_test(void);
void aevrospoint_full_test(void);

#endif