#pragma once 
#include <stdint.h>

void destroy_user_space(uint32_t cr3);

uint32_t create_user_space(void);

