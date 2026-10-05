#ifndef ROLODEX_H
#define ROLODEX_H

#include <stdint.h>

#define ROLODEX_CAPACITY 32
#define ARP_PACKET_SIZE 28
#define ARP_REQUEST 1
#define ARP_REPLY 2
#define ROLODEX_ENTRY_TIMEOUT_TICKS 6000

typedef struct
{
    uint8_t ip[4], mac[6];
    uint32_t last_seen;
    int in_use;
    int disputed;
} rolodex_entry_t;

void rolodex_handle(const uint8_t *payload, uint16_t length, const uint8_t src_mac[6]);

void rolodex_learn(const uint8_t ip[4], const uint8_t mac[6]);

int rolodex_lookup(const uint8_t ip[4], uint8_t out_mac[6]);

int rolodex_disputed(const uint8_t ip[4]);

void rolodex_set_ip(const uint8_t ip[4]);

int rolodex_build_reply(uint8_t out_buf[ARP_PACKET_SIZE], const uint8_t our_mac[6]);

int rolodex_dispatch_reply(const uint8_t our_mac[6], uint32_t *out_pass_id);

int rolodex_dispatch_request(const uint8_t target_ip[4], const uint8_t our_mac[6], uint32_t *out_pass_id);

const uint8_t *rolodex_last_request_frame(void);

uint16_t rolodex_last_request_len(void);

uint32_t rolodex_count(void);

uint32_t rolodex_contradiction_count(void);

void rolodex_get_ip(uint8_t out_ip[4]);

#endif