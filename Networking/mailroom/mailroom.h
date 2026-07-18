#ifndef MAILROOM_H
#define MAILROOM_H

#include <stdint.h>

#define ETHERTYPE_ARP  0x0806
#define ETHERTYPE_IPV4 0x0800

typedef enum
{
    MAIL_DELIVERED,      
    MAIL_UNKNOWN_TYPE,     
    MAIL_LEGACY_REJECTED,  
} mail_result_t;


mail_result_t mailroom_deliver(const uint8_t *frame, uint16_t length, const uint8_t our_mac[6]);

uint32_t mailroom_log_count(void);
uint32_t mailroom_undelivered_count(void);




#endif