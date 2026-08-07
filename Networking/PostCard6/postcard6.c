#include "postcard6.h"
#include "../Compass6/ip6_directory.h"
#include "../Rolodex6/rolodex6.h"
#include "../Bailiff/bailiff.h"
#include "../LockBox/lockbox.h"
#include "../Postbox/postbox.h"
#include "../Menu/menu.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"

static uint32_t accepted, rejected;

static uint16_t udp6_checksum(const uint8_t *src_ip, const uint8_t *dst_ip, const uint8_t *udp, uint16_t udp_len)
{
    uint32_t sum = 0;

    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((src_ip[i] << 8) | src_ip[i + 1]);
    for (int i = 0; i < 16; i += 2)
        sum += (uint16_t)((dst_ip[i] << 8) | dst_ip[i + 1]);

    sum += (udp_len >> 16) & 0xFFFF;
    sum += udp_len & 0xFFFF;
    sum += 17;

    for (int i = 0; i < udp_len; i += 2)
        sum += (uint16_t)((udp[i] << 8) | (i + 1 < udp_len ? udp[i + 1] : 0));

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(0xFFFF - sum);
}

static udp6_verdict_t postcard6_check(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    if (length < UDP6_HEADER_LEN)
        return UDP6_REJECT_TOO_SHORT;

    uint16_t declared_len = (uint16_t)((payload[4] << 8) | payload[5]);
    if (declared_len < UDP6_HEADER_LEN || declared_len > length)
        return UDP6_REJECT_LENGTH_MISMATCH;

    uint16_t declared_checksum = (uint16_t)((payload[6] << 8) | payload[7]);
    if (declared_checksum == 0)
        return UDP6_REJECT_BAD_CHECKSUM;

    if (udp6_checksum(src_ip, dst_ip, payload, declared_len) != 0)
        return UDP6_REJECT_BAD_CHECKSUM;

    return UDP6_ACCEPT;
}

void postcard6_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[16], const uint8_t dst_ip[16])
{
    udp6_verdict_t v = postcard6_check(payload, length, src_ip, dst_ip);

    if (v != UDP6_ACCEPT)
    {
        kprintf("[PostCard6] rejected: %s\n", udp6_verdict_string(v));
        rejected++;
        return;
    }

    accepted++;

    uint16_t src_port = (uint16_t)((payload[0] << 8) | payload[1]);
    uint16_t dst_port = ((payload[2] << 8) | payload[3]);

    kprintf("[Postcard6] accepted, port %d -> port %d\n", src_port, dst_port);

    if (!menu_is_open(dst_port, 17))
    {
        kprintf("[Postcard6] port %d isn't on the Menu, refusing regardless of any listener\n", dst_port);
        return;
    }

    uint32_t slot = lockbox_find_listener(dst_port, 17);
    if (slot == LOCKBOX_CAPACITY)
    {
        kprintf("[Postcard6] nobody's listening on port %d, discarding\n", dst_port);
        return;
    }
    uint16_t udp_payload_len = (uint16_t)((payload[4] << 8) | payload[5]);

    static uint8_t zero_ip4[4] = {0, 0, 0, 0};

    postbox_deposit(slot, zero_ip4, src_ip, payload + UDP6_HEADER_LEN, udp_payload_len);
}

int postcard6_dispatch(const uint8_t dest_ip[16], uint16_t dest_port, uint16_t src_port, const uint8_t our_mac[6],
                       const uint8_t our_ip[16], const uint8_t *data, uint16_t len)
{
    if (len > POSTCARD6_MAX_PAYLOAD)
        return 0;

    uint8_t dest_mac[6];
    if (!rolodex6_lookup(dest_ip, dest_mac))
    {
        kprintf("[Postcard6] don't know this neighbor's MAC yet and there's no Foyer6 to wait for Neighbor Discovery - dropping\n");
        return 0;
    }

    static uint8_t frame[6 + 6 + 2 + 40 + UDP6_HEADER_LEN + POSTCARD6_MAX_PAYLOAD];

    uint16_t udp_len = (uint16_t)(UDP6_HEADER_LEN + len);

    uint8_t *ip = frame + 14;
    uint8_t *udp = ip + 40;

    udp[0] = src_port >> 8;
    udp[1] = src_port & 0xFF;
    udp[2] = dest_port >> 8;
    udp[3] = dest_port & 0xFF;

    udp[4] = udp_len >> 8;
    udp[5] = udp_len & 0xFF;
    udp[6] = 0;
    udp[7] = 0;
    memcpy(udp + UDP6_HEADER_LEN, data, len);

    uint16_t csum = udp6_checksum(our_ip, dest_ip, udp, udp_len);
    if (csum == 0)
        csum = 0xFFFF;

    udp[6] = csum >> 8;
    udp[7] = csum & 0xFF;

    ip[0] = 0x60;
    ip[1] = 0;

    ip[2] = 0;
    ip[3] = 0;
    ip[4] = udp_len >> 8;
    ip[5] = udp_len & 0xFF;

    ip[6] = 17;
    ip[7] = 64;
    memcpy(ip + 8, our_ip, 16);
    memcpy(ip + 24, dest_ip, 16);

    memcpy(frame, dest_mac, 6);

    memcpy(frame + 6, our_mac, 6);
    frame[12] = 0x86;
    frame[13] = 0xDD;

    uint16_t total_len = (uint16_t)(14 + 40 + udp_len);

    uint32_t pass_id;
    if (bailiff_request_pass(frame, total_len, &pass_id) && bailiff_present_pass(pass_id, frame, total_len))
    {
        kprintf("[Postcard6] sent %u bytes to port %d\n", len, dest_port);
        return 1;
    }

    return 0;
}

uint32_t postcard6_accepted_count(void)
{
    return accepted;
}

uint32_t postcard6_rejected_count(void)
{
    return rejected;
}

const char *udp6_verdict_string(udp6_verdict_t v)
{
    switch (v)
    {
    case UDP6_ACCEPT:
        return "ACCEPT";
    case UDP6_REJECT_TOO_SHORT:
        return "REJECT (too short)";
    case UDP6_REJECT_LENGTH_MISMATCH:
        return "REJECT (length lied)";
    case UDP6_REJECT_BAD_CHECKSUM:
        return "REJECT (checksum failed)";
    default:
        return "REJECT (unknown)";
    }
}

IP6_DIRECTORY_ENTRY(17, postcard6_handle, "Postcard6 (UDP6)");
