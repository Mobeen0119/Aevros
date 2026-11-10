#include "../../IPv6/Frontdoor6/frontdoor6.h"
#include "../../../kernel/Process/task.h"
#include "../../../Lib/kprintf.h"
#include "../../../Lib/string.h"
#include "../../../Include/vfs.h"
#include "fetch.h"

#define FETCH_BUF_MAX 65536
#define FETCH_TIMEOUT_TICKS 800

static uint8_t fetch_buf[FETCH_BUF_MAX];
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

static void fetch_task(void)
{
    int n = 0;
    fetch_request[0] = 0;

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
    uint16_t req_len = (uint16_t)n;
    int sent = 0;

    while (sent < 0 || (sent == 0 && get_ticks() - started < FETCH_TIMEOUT_TICKS))
    {
        int r = frontdoor_send(fd, (const uint8_t *)fetch_request, req_len);
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

    uint32_t total = 0;
    uint32_t idle_since = get_ticks();

    for (;;)
    {
        int got = frontdoor_recv(fd, fetch_buf + total, (uint16_t)(FETCH_BUF_MAX - total > 1400 ? 1400 : FETCH_BUF_MAX - total));

        if (got < 0)
            break;

        if (got > 0)
        {
            total += (uint32_t)got;
            idle_since = get_ticks();
            if (total >= FETCH_BUF_MAX)
                break;
        }
        else
        {
            if (get_ticks() - idle_since > FETCH_TIMEOUT_TICKS)
                break;

            fetch_wait(1);
        }
    }

    frontdoor_close(fd);
    kprintf("[fetch] received %u bytes\n", total);

    uint32_t body_at = 0;

    for (uint32_t i = 0; i + 3 < total; i++)
    {
        if (fetch_buf[i] == '\r' && fetch_buf[i + 1] == '\n' && fetch_buf[i + 2] == '\r' && fetch_buf[i + 3] == '\n')
        {
            body_at = i + 4;
            break;
        }
    }

    if (total < 12 || fetch_buf[9] != '2' || fetch_buf[10] != '0' || fetch_buf[11] != '0')
    {
        kprintf("[fetch] server did not return 200 OK, nothing saved\n");
        sys_exit(1);
    }

    int out = sys_open(fetch_save_as, READ_WRITE | CREAT);
    if (out < 0)
    {
        kprintf("[fetch] could not create %s\n", fetch_save_as);
        sys_exit(1);
    }
    uint32_t body_len = total - body_at;
    sys_write(out, fetch_buf + body_at, body_len);
    sys_close(out);
    kprintf("[fetch] saved %u bytes to %s\n", body_len, fetch_save_as);
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