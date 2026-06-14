#ifndef KALLOC_TRACKER_H
#define KALLOC_TRACKER_H

#include <stdint.h>
#include <stddef.h>

#define TRACKER_MAX_ALLOCS 512

typedef struct {
    void* ptr;
    size_t size;
    const char* file,*func;

    uint32_t line,pid;
    uint8_t alive;
}alloc_record_t;

void tracker_init(void);
void tracker_record(void* ptr,size_t size,const char* file,uint32_t line,const char* func);

void tracker_remove(void* ptr);
void tracker_dump(void);
uint32_t tracker_live_count(void);

uint32_t tracker_leaked_bytes(void);

#endif