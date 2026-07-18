#include "bouncer.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/string.h"

#define MIN_ETHERNET_FRAME 14
#define MAX_ETHERNET_FRAME 1518

static bouncer_log_entry_t door_log[BOUNCER_LOG_CAPACITY];
static uint32_t log_head;
static uint32_t log_total;
static uint32_t turned_away;

static mac_equal(const uint8_t *a, const uint8_t *b)
{
    for (int i = 0; i < 6; i++)
        if (a[i] != b[i])
            return 0;

    return 1;
}

static mac_broadcast(const uint8_t *mac)
{
    for (int i = 0; i < 6; i++)
        if (mac[i] != 0xFF)
            return 0;
    return 1;
}

static mac_multicast(const uint8_t *mac)
{
    return (mac[0] & 0x01) != 0;
}

bounce_verdict_t bouncer_check(const uint8_t *frame, uint16_t length, const uint8_t our_mac[6])
{
    if (length > MAX_ETHERNET_FRAME)
        return BOUNCE_REJECT_TOO_LONG;
    if (length < MIN_ETHERNET_FRAME)
        return BOUNCE_REJECT_TOO_SHORT;

    uint8_t *dest = frame;

    if (mac_equal(dest, our_mac))
        return BOUNCE_ACCEPT;
    if (mac_broadcast(dest))
        return BOUNCE_ACCEPT;
    if (mac_multicast(dest))
        return BOUNCE_ACCEPT;

    return BOUNCE_REJECT_BAD_DEST;
}

void bouncer_log(bounce_verdict_t verdict, uint16_t length, const uint8_t *dest_mac, const uint8_t *src_mac)
{
    bouncer_log_entry_t *e = &door_log[log_head];
    e->tick = get_ticks();
    e->verdict = verdict;
    e->length = length;

    if (dest_mac)
        memcpy(e->dest_mac, dest_mac, 6);
    if (src_mac)
        memcpy(e->src_mac, src_mac, 6);

    log_head = (log_head + 1) % BOUNCER_LOG_CAPACITY;
    log_total++;

    if (verdict != BOUNCE_ACCEPT)
        turned_away++;
}

uint32_t bouncer_log_count(void){
    return log_total < BOUNCER_LOG_CAPACITY ? log_total  : BOUNCER_LOG_CAPACITY;
}

const bouncer_log_entry_t *bouncer_log_get(uint32_t index){
    if(index >= bouncer_log_count()) return 0;

    uint32_t pos=(log_head + BOUNCER_LOG_CAPACITY -1 -index) % BOUNCER_LOG_CAPACITY;
    
    return &door_log[pos];
}

uint32_t bouncer_turned_away_count(void) { return turned_away; }

const char *bounce_verdict_string(bounce_verdict_t v){

    switch(v){
        case BOUNCE_ACCEPT : return "LET IN";
        case BOUNCE_REJECT_BAD_DEST : return "TURNED_AWAY (Not in the List )";
        case BOUNCE_REJECT_TOO_SHORT:  return "TURNED AWAY (too short)";
        case BOUNCE_REJECT_TOO_LONG:   return "TURNED AWAY (too long)";

        default : return "TURNED AWAY(unknown)";
        
    }
}