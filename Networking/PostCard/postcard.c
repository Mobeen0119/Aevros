#include "postcard.h"
#include "../Compass/ip_directory.h"
#include "../../Lib/kprintf.h"
#include "../../Lib/string.h"
#include "../LockBox/lockbox.h"
#include "../Postbox/postbox.h"
#include "../Menu/menu.h"
#include "../Bailiff/bailiff.h"
#include "../Foyer/foyer.h"
#include "../Atlas/atlas.h"

#define UDP_HEADER_LEN 8

static uint32_t accepted, rejected;

static int checksum_ok(const uint8_t *udp, const uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4])
{

    uint16_t given = (uint16_t)(udp[6] << 8 | udp[7]);

    if (given == 0)
        return 1;

    uint32_t sum = 0;

    for (int i = 0; i < 4; i += 2)
        sum += (uint16_t)((src_ip[i] << 8) | src_ip[i + 1]);
    for (int i = 0; i < 4; i += 2)
        sum += (uint16_t)((dst_ip[i] << 8) | dst_ip[i + 1]);

    sum += 17;
    for (int i = 0; i < length; i += 2)
    {
        uint16_t word = (uint16_t)((udp[i] << 8) | (i + 1 < length ? udp[i + 1] : 0));
        sum += word;
    }
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);
    return sum == 0xFFFF;
}

static udp_verdict_t postcard_check(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4], uint16_t *out_declared_len)
{
    if (length < UDP_HEADER_LEN)
        return UDP_REJECT_TOO_SHORT;

    uint16_t declared_len = (uint16_t)((payload[4] << 8) | payload[5]);
    *out_declared_len = declared_len;

    if (declared_len < UDP_HEADER_LEN || declared_len > length)
        return UDP_REJECT_LENGTH_MISMATCH;

    if (!checksum_ok(payload, declared_len, src_ip, dst_ip))
        return UDP_REJECT_BAD_CHECKSUM;

    return UDP_ACCEPT;
}

void postcard_handle(const uint8_t *payload, uint16_t length, const uint8_t src_ip[4], const uint8_t dst_ip[4])
{

    uint16_t declared_len = 0;
    udp_verdict_t v = postcard_check(payload, length, src_ip, dst_ip, &declared_len);

    if (v != UDP_ACCEPT)
    {
        kprintf("[Postcard] rejected: %s\n", udp_verdict_string(v));
        rejected++;
        return;
    }

    accepted++;

    uint16_t src_port = (uint16_t)((payload[0] << 8) | payload[1]);
    uint16_t dst_port = (uint16_t)((payload[2] << 8) | payload[3]);

    kprintf("[Postcard] accepted, %d.%d.%d.%d:%d -> %d.%d.%d.%d:%d\n",
            src_ip[0], src_ip[1], src_ip[2], src_ip[3], src_port,
            dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], dst_port);

    if (!menu_is_open(dst_port, 17))
    {
        kprintf("[Postcard] port %d isn't on the Menu, refusing regardless of any listener\n", dst_port);
        return;
    }

    uint32_t slot = lockbox_find_listener(dst_port, 17);

    if (slot == LOCKBOX_CAPACITY)
    {
        kprintf("[Postcard] nobody's listening on port %d, discarding\n", dst_port);
        return;
    }

    uint16_t data_len = (uint16_t)(declared_len - UDP_HEADER_LEN);

    if (!postbox_deposit(slot, src_ip, src_port, payload + UDP_HEADER_LEN, data_len))
        return;

    kprintf("[Postcard] delivered %d bytes into slot %d\n", data_len, slot);
}

int postcard_dispatch(const uint8_t dest_ip[4], uint16_t dest_port, uint16_t src_port, const uint8_t our_mac[6], const uint8_t our_ip[4],
                      const uint8_t *data, uint16_t len)
{
    if (len > POSTCARD_MAX_PAYLOAD)
        return 0;

    static int frame[14 + 20 + UDP_HEADER_LEN + POSTCARD_MAX_PAYLOAD];

    uint16_t udp_len = (uint16_t)(UDP_HEADER_LEN + len);
    uint16_t ip_total = (uint16_t)(20 + udp_len);

    uint8_t *ip = frame + 14;
    uint8_t *udp = ip + 20;

    udp[0] = src_port >> 8;
    udp[1] = src_port & 0xFF;
    udp[2] = dest_port >> 8;
    udp[3] = dest_port & 0xFF;

    udp[4] = udp_len >> 8;
    udp[5] = udp_len & 0xFF;
    udp[6] = 0;
    udp[7] = 0;

    memcpy(udp + UDP_HEADER_LEN, data, len);

    uint32_t sum = 0;

    for (int i = 0; i < 4; i += 2)
        sum += (uint16_t)((our_ip[i] << 8) | our_ip[i + 1]);
    for (int i = 0; i < 4; i += 2)
        sum += (uint16_t)((dest_ip[i] << 8) | dest_ip[i + 1]);

    sum += 17;

    for (int i = 0; i < udp_len; i += 2)
        sum += (uint16_t)((udp[i] << 8) | i + 1 < udp_len ? udp[i + 1] : 0);
    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    uint16_t udp_csum = (uint16_t)(0xFFFF - sum);

    if (udp_csum == 0)
        udp_csum = 0xFFFF; // No Checksum

    udp[6] = udp_csum >> 8;
    udp[7] = udp_csum & 0xFF;

    ip[0] = 0x45;
    ip[1] = 0;
    ip[2] = ip_total >> 8;
    ip[3] = ip_total & 0xFF;
    ip[4] = 0;

    ip[5] = 0;
    ip[6] = 0;
    ip[7] = 0;
    ip[8] = 64;

    ip[9] = 17;
    ip[10] = 0;
    ip[11] = 0;

    memcpy(ip + 12, our_ip, 4);
    memcpy(ip + 16, dest_ip, 4);

    uint32_t ip_sum = 0;

    for (int i = 0; i < 20; i += 2)
        ip_sum += (uint16_t)((ip[i] << 8) | ip[i + 1]);

    while (ip_sum >> 16)
        ip_sum = (ip_sum & 0xFFFF) + (ip_sum >> 16);

    uint16_t ip_csum = (uint16_t)(0xFFFF - ip_sum);

    ip[10] = ip_csum >> 8;
    ip[11] = ip_csum & 0xFF;

    frame[12] = 0x08;
    frame[13] = 0x00;

    memcpy(frame + 6, our_mac, 6);

    uint16_t total_len = (uint16_t)(14 + ip_total);

    int is_broadcast = (dest_ip[0] == 255 && dest_ip[1] == 255 && dest_ip[2] == 255 && dest_ip[3] == 255);

    if (is_broadcast)
    {

        memset(frame, 0xFF, 6);
        uint32_t pass_id;
        if (bailiff_request_pass(frame, total_len, &pass_id) && bailiff_present_pass(pass_id, frame, total_len))
        {
            kprintf("[Postcard] broadcast %u bytes to 255.255.255.255:%d\n", len, dest_port);
            return 1;
        }
        return 0;
    }

    uint8_t next_hop[4];

    if (!atlas_lookup(dest_ip, next_hop))
        return 0;

    uint8_t dest_mac[6];

    if (rolodex_lookup(next_hop, dest_mac))
    {
        memcpy(frame, dest_mac, 6);

        uint32_t pass_id;

        if (bailiff_request_pass(frame, total_len, &pass_id) && bailiff_present_pass(pass_id, frame, total_len))
        {
            kprintf("[Postcard] sent %u bytes to %d.%d.%d.%d:%d\n", len, dest_ip[0], dest_ip[1], dest_ip[2], dest_ip[3], dest_port);
            return 1;
        }

        return 0;
    }

    memset(frame, 0, 6);

    return foyer_queue(next_hop, our_mac, frame, total_len);
}

uint32_t postcard_accepted_count(void)
{
    return accepted;
}

uint32_t postcard_rejected_count(void)
{
    return rejected;
}

const char *udp_verdict_string(udp_verdict_t v)
{
    switch (v)
    {
    case UDP_ACCEPT:
        return "ACCEPT";
    case UDP_REJECT_TOO_SHORT:
        return "REJECT (too short)";
    case UDP_REJECT_LENGTH_MISMATCH:
        return "REJECT (length lied)";
    case UDP_REJECT_BAD_CHECKSUM:
        return "REJECT (checksum failed)";
    default:
        return "REJECT (unknown)";
    }
}

IP_DIRECTORY_ENTRY(17, postcard_handle, "Postcard (UDP)");