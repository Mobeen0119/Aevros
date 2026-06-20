#ifndef KALLOC_TRACKER_H
#define KALLOC_TRACKER_H

#include <stdint.h>
#include <stddef.h>

#define TRACKER_MAX_ALLOCS 2048

typedef struct
{
    void *ptr;
    size_t size;
    const char *file;
    const char *func;
    uint32_t line;
    uint32_t pid;
    uint32_t timestamp;
    uint8_t alive;
} alloc_record_t;

void tracker_init(void);
void tracker_record(void *ptr, size_t size, const char *file, uint32_t line, const char *func);

void tracker_remove(void *ptr);
void tracker_dump(void);
uint32_t tracker_live_count(void);

uint32_t tracker_leaked_bytes(void);
void tracker_dump_pid(uint32_t pid);
void tracker_dump_ghosts(void);

alloc_record_t *tracker_find(void* ptr);

alloc_record_t* tracker_get_table(void);

uint32_t tracker_table_size(void);

#endif