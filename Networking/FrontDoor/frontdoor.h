#ifndef FRONTDOOR_H
#define FRONTDOOR_H

#include <stdint.h>

#define FRONTDOOR_MAX_SOCKETS 32
#define SOCK_TYPE_TCP 1
#define SOCK_TYPE_UDP 2

typedef struct
{
    uint8_t ip[4];
    uint16_t port;
} sock_addr_t;

typedef struct
{
    sock_addr_t dest;
    const uint8_t *buf;
    uint16_t len;

} sendto_args_t;

typedef struct
{
    sock_addr_t src_out;
    uint8_t *buf;
    uint16_t max_len;

} recvfrom_args_t;

void frontdoor_init(void);

void frontdoor_tick(void);

int frontdoor_socket(int type);

int frontdoor_bind(int fd, uint16_t port);

int frontdoor_connect(int fd, const sock_addr_t *addr);

int frontdoor_accept(int fd);

int frontdoor_send(int fd, const uint8_t *buf, uint16_t len);

int frontdoor_recv(int fd, uint8_t *buf, uint16_t max_len);

int frontdoor_sendto(int fd, const sendto_args_t *args);

int frontdoor_recvfrom(int fd, recvfrom_args_t *args);

int frontdoor_close(int fd);

#endif