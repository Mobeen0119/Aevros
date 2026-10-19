#include "../IPv6/Frontdoor6/frontdoor6.h"
#include "../Menu/menu.h"
#include "../../kernel/Process/task.h"
#include "../../Lib/kprintf.h"
#include "tcpecho.h"

#define ECHO_PORT 7007

static void wait_ticks(uint32_t n)
{
    uint32_t start = get_ticks();
    while (get_ticks() - start < n)
        ;
}

static void echo_task(void)
{
    // stack refuses any port that is not on the Menu
    menu_open_port(ECHO_PORT, 6);

    int listener = frontdoor_socket(SOCK_TYPE_TCP);

    if (listener < 0 || frontdoor_bind(listener, ECHO_PORT) < 0)
    {
        kprintf("[tcpecho] could not listen on port %d\n", ECHO_PORT);
        sys_exit(1);
    }

    kprintf("[tcpecho] listening on port %d\n", ECHO_PORT);

    for (;;)
    {
        int conn = frontdoor_accept(listener);

        if (conn < 0)
        {
            wait_ticks(5);
            continue;
        }

        kprintf("[tcpecho] connection accepted\n");

        for (;;)
        {
            uint8_t buf[128];
            int got = frontdoor_recv(conn, buf, sizeof(buf));

            if (got < 0)
                break; // the peer closed

            if (got > 0)
            {
                // stack keeps one unacknowledged segment in flight.......send can be refused until the ACK arrives
                int sent = 0;
                uint32_t started = get_ticks();

                while (sent < got && get_ticks() - started < 500)
                {
                    int r = frontdoor_send(conn, buf + sent, (uint16_t)(got - sent));

                    if (r > 0)
                        sent += r;
                    else
                        wait_ticks(1);
                }
            }
            else
                wait_ticks(1);
        }

        frontdoor_close(conn);
        kprintf("[tcpecho] connection closed\n");
    }
}

void tcpecho_start(void)
{
    if (!task_create_kernel(echo_task))
        kprintf("[tcpecho] could not start the echo task\n");
}