#ifndef ROLODEX6_H
#define ROLODEX6_H

#include <stdint.h>

#define ROLODEX6_CAPACITY 16
#define ROLODEX6_ENTRY_TIMEOUT_TICKS 6000

void rolodex6_set_ip(const uint8_t ip[16]);

void rolodex6_get_ip(uint8_t out_ip[16]);

int rolodex6_have_ip(void);

void rolodex6_learn(const uint8_t ip[16], const uint8_t mac[6]);

int rolodex6_lookup(const uint8_t ip[16], uint8_t out_mac[6]);

int rolodex6_disputed(const uint8_t ip[16]);

uint32_t rolodex6_contradiction_count(void);

void rolodex6_tick(void);

uint32_t rolodex6_entry_count(void);

#endif