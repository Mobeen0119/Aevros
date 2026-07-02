#include "keyboard.h"
#include "../kernel/io.h"
#include "../Include/terminal.h"

#define KEY_RELEASE    0x80

#define SCAN_HOME      0x47
#define SCAN_UP        0x48
#define SCAN_PAGE_UP   0x49

#define SCAN_LEFT      0x4B
#define SCAN_RIGHT     0x4D

#define SCAN_END       0x4F
#define SCAN_DOWN      0x50
#define SCAN_PAGE_DOWN 0x51

unsigned char keyb[128] =
{
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0
};

volatile int keyb_head=0,keyb_tail=0;
volatile char keyboard_buffer[128];

void keyboard_push(char c)
{
    int next=(keyb_head+1)&127;
    if(next==keyb_tail)
        return;

    keyboard_buffer[keyb_head]=c;
    keyb_head=next;
}

volatile char keyboard_getchar()
{
    while(keyb_head==keyb_tail)
        asm volatile("hlt");

    char c=keyboard_buffer[keyb_tail];
    keyb_tail=(keyb_tail+1)&127;
    return c;
}

void keyboard_handler()
{
    uint8_t scan=inb(0x60);

    if(scan&KEY_RELEASE)
        return;

    switch(scan)
    {
        case SCAN_UP:
            if(scroll_top>0)
            {
                scroll_top--;
                render();
            }
            return;

        case SCAN_DOWN:
            if(scroll_top<cursor_y)
            {
                scroll_top++;
                render();
            }
            return;

        case SCAN_PAGE_UP:

            scroll_top-=20;

            if(scroll_top<0)
                scroll_top=0;

            render();
            return;

        case SCAN_PAGE_DOWN:

            scroll_top+=20;

            if(scroll_top>cursor_y)
                scroll_top=cursor_y;

            render();
            return;

        case SCAN_HOME:

            scroll_top=0;
            render();
            return;

        case SCAN_END:

            if(cursor_y>=HEIGHT)
                scroll_top=cursor_y-HEIGHT+1;
            else
                scroll_top=0;

            render();
            return;
    }

    char c=keyb[scan];

    if(c)
        keyboard_push(c);
}
