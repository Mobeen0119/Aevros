#pragma once

#include <stdint.h>
#define USER_STACK_TOP 0xBFFFF000
#define USER_STACK_SIZE PAGE_SIZE

int exec_user(void* binary,uint32_t size);

int sys_exec(const char* path);