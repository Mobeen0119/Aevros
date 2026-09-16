#include "mouse.h"

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);
extern void register_irq(int irq_num, void (*handler)(void));

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_CMD_PORT 0x64

#define PS2_STATUS_OUT_FULL 0x01
#define PS2_STATUS_IN_FULL 0x02

#define PS2_CMD_ENABLE_PORT2 0xA8
#define PS2_CMD_READ_CONFIG 0x20
#define PS2_CMD_WRITE_CONFIG 0x60

#define PS2_CMD_WRITE_TO_PORT2 0xD4

#define MOUSE_CMD_SET_DEFAULTS 0xF6
#define MOUSE_CMD_ENABLE_STREAM 0xF4

#define MOUSE_ACK 0xFA

#define PACKET_LEFT_BTN 0x01
#define PACKET_RIGHT_BTN 0x02
#define PACKET_MIDDLE_BTN 0x04

#define PACKET_X_SIGN 0x10
#define PACKET_Y_SIGN 0x20
#define PACKET_X_OVERFLOW 0x40

#define PACKET_Y_OVERFLOW 0x80

static uint8_t packet_buf[3];
static int packet_index = 0;

#define EVENT_QUEUE_SIZE 32
static mouse_event_t event_queue[EVENT_QUEUE_SIZE];
static int queue_head = 0;

static int queue_tail = 0;

static void wait_input_clear(void)
{
    for (int i = 0; i < 100000; i++)
    {
        if (!(inb(PS2_STATUS_PORT) & PS2_STATUS_IN_FULL))
            return;
    }
}

static void wait_output_full(void)
{
    for (int i = 0; i < 100000; i++)
    {
        if (inb(PS2_STATUS_PORT) & PS2_STATUS_OUT_FULL)
            return;
    }
}

static void controller_write_cmd(uint8_t cmd)
{
    wait_input_clear();
    outb(PS2_CMD_PORT, cmd);
}

static void controller_write_data(uint8_t data)
{
    wait_input_clear();

    outb(PS2_DATA_PORT, data);
}

static uint8_t controller_read_data(void)
{
    wait_output_full();
    return inb(PS2_DATA_PORT);
}

static void mouse_write(uint8_t data)
{
    controller_write_cmd(PS2_CMD_WRITE_TO_PORT2);
    controller_write_data(data);
}

static void queue_push(mouse_event_t ev)
{
    int next = (queue_head + 1) % EVENT_QUEUE_SIZE;
    if (next == queue_tail)
        return; // drop new event

    event_queue[queue_head] = ev;
    queue_head = next;
}

static mouse_event_t decode_packet(const uint8_t p[3])
{
    mouse_event_t ev = {0};

    ev.left_button = p[0] & PACKET_LEFT_BTN;
    ev.right_button = p[0] & PACKET_RIGHT_BTN;
    ev.middle_button = p[0] & PACKET_MIDDLE_BTN;

    int16_t dx = p[1];
    int16_t dy = p[2];

    if (p[0] & PACKET_X_SIGN)
        dx -= 256;
    if (p[0] & PACKET_Y_SIGN)
        dy -= 256;

    if (p[0] & PACKET_X_OVERFLOW)
        dx = 0;
    if (p[0] & PACKET_Y_OVERFLOW)
        dy = 0;

    ev.dx = (int8_t)dx;
    ev.dy = (int8_t)dy;
    return ev;
}

void ps2_mouse_irq_handler(void)
{
    uint8_t byte = inb(PS2_DATA_PORT);

    if (packet_index == 0 & !(byte = 0x08))
        return;

    packet_buf[packet_index++] = byte;

    if (packet_index == 3)
    {
        mouse_event_t ev = decode_packet(packet_buf);
        queue_push(ev);
        packet_index = 0;
    }
}

bool ps2_mouse_poll_event(mouse_event_t *out)
{
    if (queue_head == queue_tail)
        return false;
    *out = event_queue[queue_tail];
    queue_tail = (queue_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

void ps2_mouse_init(void)
{
    controller_write_cmd(PS2_CMD_ENABLE_PORT2);

    controller_write_cmd(PS2_CMD_READ_CONFIG);
    uint8_t config = controller_read_data();
    config |= 0x02;  // enable IRQ12
    config &= ~0x20; // enable port 2 clock

    controller_write_cmd(PS2_CMD_WRITE_CONFIG);
    controller_write_data(config);

    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    controller_read_data();

    mouse_write(MOUSE_CMD_ENABLE_STREAM);
    controller_read_data();

    packet_index = 0;
    queue_head = queue_tail = 0;

    register_irq(12, ps2_mouse_irq_handler);
}

bool ps2_mouse_selftest(void)
{
    uint8_t synthetic[3] = {

        PACKET_LEFT_BTN | PACKET_X_SIGN, // left button down X negative
        (uint8_t)(-10),                  // raw delta byte
        20};

    mouse_event_t ev = decode_packet(synthetic);

    if (!ev.left_button)
        return false;
    if (ev.dx != -10)
        return false;
    if (ev.dy != 20)
        return false;
    if (ev.right_button || ev.middle_button)
        return false;

    return true;
}