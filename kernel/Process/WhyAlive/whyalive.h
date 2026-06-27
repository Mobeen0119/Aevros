#ifndef WHYALIVE_H
#define WHYALIVE_H
#include <stdint.h>

void whyalive_inode_path(const char* path);
void whyalive_task(uint32_t pid);
void whyalive_alloc(void* ptr);


#endif