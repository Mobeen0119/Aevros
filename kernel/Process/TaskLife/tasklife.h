#ifndef TASKLIFE_H
#define TASK_LIFE_H

#include "../task.h"

void task_log_event(task_t* task,task_event_type_t type,uint32_t data);

void tasklife_dump(uint32_t pid);

void tasklife_dump_current(void);

void tasklife_ps(void);

#endif