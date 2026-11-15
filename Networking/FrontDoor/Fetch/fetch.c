#include "../../IPv6/Frontdoor6/frontdoor6.h"
#include "../../../kernel/Process/task.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
#include "../../../Include/vfs.h"
#include "fetch.h"
#include <stdint.h>

#define FETCH_HEADER_MAX 1024
#define FETCH_CHUNK 4096
#define FETCH_TIMEOUT_TICKS 800

static uint8_t fetch_head[FETCH_HEADER_MAX];
static uint8_t fetch_chunk[FETCH_CHUNK];
static const uint8_t fetch_ip[4] = {10, 0, 2, 2};
static const uint16_t fetch_port = 8000;
static char fetch_path[64] = "/hello.txt";
static char fetch_save_as[64] = "/hello.txt";
static char fetch_request[160];

static void fetch_wait(uint32_t n)
{
    uint32_t start = get_ticks();

    while (get_ticks() - start < n)
        ;
}

static uint32_t fnv_feed(uint32_t sum, const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
        sum = (sum ^ data[i]) * 16777619u;
    return sum;
}

static int find_header_end(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i + 3 < len; i++)
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
            return (int)i + 4;
    return -1;
}

static void fetch_task(void)
{
    int n = 0;

    const char *head = "GET ";

    while (*head)
        fetch_request[n++] = *head++;

    for (const char *p = fetch_path; *p && n < 120; p++)
        fetch_request[n++] = *p;

    const char *tail = " HTTP/1.0\r\nHost: 10.0.2.2\r\n\r\n";

    while (*tail && n < 159)
        fetch_request[n++] = *tail++;
    fetch_request[n] = 0;

    int fd = frontdoor_socket(SOCK_TYPE_TCP);
    if (fd < 0)
    {
        kprintf("[fetch] no socket\n");
        sys_exit(1);
    }

    sock_addr_t dest;
    memcpy(dest.ip, fetch_ip, 4);
    dest.port = fetch_port;

    if (frontdoor_connect(fd, &dest) < 0)
    {
        kprintf("[fetch] connect refused\n");

        frontdoor_close(fd);
        sys_exit(1);
    }
    kprintf("[fetch] connecting to 10.0.2.2:%d\n", fetch_port);

    uint32_t started = get_ticks();
    int sent = 0;

    while (sent == 0 && get_ticks() - started < FETCH_TIMEOUT_TICKS)
    {
        int r = frontdoor_send(fd, (const uint8_t *)fetch_request, (uint16_t)n);
        if (r > 0)
            sent = r;
        else
            fetch_wait(2);
    }
    if (sent <= 0)
    {
        kprintf("[fetch] could not send the request\n");
        frontdoor_close(fd);
        sys_exit(1);
    }
    kprintf("[fetch] request sent (%d bytes)\n", sent);

    uint32_t head_len = 0;
    int header_done = 0;
    int out = -1;
    uint32_t body_len = 0;
    uint32_t sum = 2166136261u;
    uint32_t idle_since = get_ticks();

    for (;;)
    {
        int got = frontdoor_recv(fd, fetch_chunk, FETCH_CHUNK);
        if (got < 0)
            break;

        if (got == 0)
        {
            if (get_ticks() - idle_since > FETCH_TIMEOUT_TICKS)
                break;
            fetch_wait(1);
            continue;
        }
        idle_since = get_ticks();

        uint32_t offset = 0;
        if (!header_done)
        {
            uint32_t room = FETCH_HEADER_MAX - head_len;
            uint32_t take = (uint32_t)got < room ? (uint32_t)got : room;
            memcpy(fetch_head + head_len, fetch_chunk, take);
            head_len += take;

            int end = find_header_end(fetch_head, head_len);
            if (end < 0)
            {
                if (head_len == FETCH_HEADER_MAX)
                {
                    kprintf("[fetch] headers too long, nothing saved\n");
                    frontdoor_close(fd);
                    sys_exit(1);
                }
                continue;
            }

            if (head_len < 12 || fetch_head[9] != '2' || fetch_head[10] != '0' || fetch_head[11] != '0')
            {
                kprintf("[fetch] server did not return 200 OK, nothing saved\n");
                frontdoor_close(fd);
                sys_exit(1);
            }

            out = sys_open(fetch_save_as, READ_WRITE | CREAT);
            if (out < 0)
            {
                kprintf("[fetch] could not create %s\n", fetch_save_as);
                frontdoor_close(fd);
                sys_exit(1);
            }
            header_done = 1;

            uint32_t body_in_head = head_len - (uint32_t)end;
            if (body_in_head > 0)
            {
                sys_write(out, fetch_head + end, body_in_head);
                sum = fnv_feed(sum, fetch_head + end, body_in_head);
                body_len += body_in_head;
            }
            offset = take;
        }

        if (offset < (uint32_t)got)
        {
            uint32_t rest = (uint32_t)got - offset;
            sys_write(out, fetch_chunk + offset, rest);
            sum = fnv_feed(sum, fetch_chunk + offset, rest);
            body_len += rest;
        }
    }

    frontdoor_close(fd);
    if (out >= 0)
        sys_close(out);

    if (!header_done)
    {
        kprintf("[fetch] no response\n");
        sys_exit(1);
    }

    kprintf("[fetch] saved %u bytes to %s, fnv1a %x\n", body_len, fetch_save_as, sum);
    sys_exit(0);
}

int fetch_start(const char *path, const char *save_as)
{
    strncpy(fetch_path, path, sizeof(fetch_path) - 1);
    fetch_path[sizeof(fetch_path) - 1] = 0;

    strncpy(fetch_save_as, save_as, sizeof(fetch_save_as) - 1);
    fetch_save_as[sizeof(fetch_save_as) - 1] = 0;

    if (!task_create_kernel(fetch_task))
    {
        kprintf("[fetch] could not start\n");
        return 0;
    }
    return 1;
}