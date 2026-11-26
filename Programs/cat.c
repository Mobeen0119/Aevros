#include "../Lib/aevros.h"

int main(void)
{
    char buf[256];

    int fd = open("/hello.txt", AEV_READ_ONLY);

    if (fd < 0)
    {
        puts("cat: /hello.txt not found (fetch it first)\n");
        return 1;
    }

    int n = read(fd, buf, sizeof(buf));

    if (n > 0)
        write(AEV_STDOUT, buf, n);

    close(fd);
    return 0;
}