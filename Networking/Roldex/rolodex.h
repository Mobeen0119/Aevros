#ifndef ROLODEX_H
#define ROLODEX_H

#include <stdint.h>

#define ROLODEX_CAPACITY 32
#define ARP_PACKET_SIZE 28

typedef struct {
    uint8_t ip[4],mac[6];
    uint32_t last_seen;
    int in_use;
}rolodex_entry_t;


void rolodex_handle(const uint8_t *payload,uint16_t length);

int rolodex_lookup(const uint8_t ip[4],uint8_t out_mac[6]);

uint32_t rolodex_count(void);
uint32_t rolodex_contradiction_count(void);

#endif