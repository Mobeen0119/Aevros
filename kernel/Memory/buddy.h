#ifndef BUDDY_H
#define BUDDY_H

#include <stddef.h>

void *buddy_alloc(int order);
void buddy_free(void *ptr, int order);

#endif // BUDDY_H
