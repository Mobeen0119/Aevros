#include "compass.h"
#include "../mailroom/directory.h"
#include "../../Lib/kprintf.h"

#include <stdint.h>

#define MIN_IP_HEADER 20

static uint32_t accepted, rejected;

static int checksum_ok(const uint8_t *header, int header_len)
{
    uint32_t sum = 0;

    for (int i = 0; i < header_len; i += 2)
    {
        uint16_t word = (uint16_t)((header[i] << 8) | (i + 1 < header_len ? header[i + 1] : 0));
        sum += word;
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return sum == 0xFFFF;
}

static ip_verdict_t compass_check(const uint8_t *payload, uint16_t length)
{
    if (length < MIN_IP_HEADER)
        return IP_REJECT_TOO_SHORT;

    uint8_t version = (uint8_t)(payload[0] >> 4);
    uint8_t ihl = (uint8_t)(payload[0] & 0x0F);

    if (version != 4)
        return IP_REJECT_BAD_VERSION;
    if (ihl < 5)
        return IP_REJECT_BAD_HEADER_LEN;

    uint16_t header_len = (uint16_t)(ihl * 4);
    if (length < header_len)
        return IP_REJECT_BAD_HEADER_LEN;

    uint16_t total_len = (uint16_t)(payload[2] << 8) | (payload[3]);
    
    if (total_len > length)
        return IP_REJECT_LENGTH_MISMATCH;

    uint16_t flags_frag = (uint16_t)(payload[6] << 8) | (payload[7]);
    uint16_t frag_offset = (uint16_t)(flags_frag & 0x1FFF);

    int more_fragments = (flags_frag >> 13) & 0x1;

    if (frag_offset != 0 || more_fragments)
        return IP_REJECT_FRAGMENTED;

    if (!checksum_ok(payload, header_len))
        return IP_REJECT_BAD_CHECKSUM;

    return IP_ACCEPT;
}

void compass_handle(const uint8_t* payload,uint16_t length){
    ip_verdict_t v=compass_check(payload,length);

    if(v!=IP_ACCEPT){
        void compass_handle(const uint8_t* payload,uint16_t length);
        rejected++;
        return;
    }

    accepted++;
    uint8_t protocol=payload[9];
    const uint8_t *src_ip=payload+12;
    const uint8_t *dst_ip=payload+16;

    kprintf("[Compass] accepted from %d.%d.%d.%d to %d.%d.%d.%d, protocol %d\n",
        src_ip[0], src_ip[1], src_ip[2], src_ip[3],
        dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], protocol);
}

uint32_t compass_accepted_count(void) { 
    return accepted;
 }

uint32_t compass_rejected_count(void) { 
    return rejected;
 }

const char *ip_verdict_string(ip_verdict_t v)
{
    switch (v)
    {
        case IP_ACCEPT: return "ACCEPT";
        case IP_REJECT_TOO_SHORT: return "REJECT (too short)";
        case IP_REJECT_BAD_VERSION: return "REJECT (not IPv4)";
        case IP_REJECT_BAD_HEADER_LEN: return "REJECT (bad header length)";
        case IP_REJECT_LENGTH_MISMATCH: return "REJECT (length lied)";
        case IP_REJECT_FRAGMENTED: return "REJECT (fragmented, not supported)";
        case IP_REJECT_BAD_CHECKSUM: return "REJECT (checksum failed)";
        default: return "REJECT (unknown)";
    }
}

DIRECTORY_ENTRY(0x0800, compass_handle, "Compass (IPv4)");