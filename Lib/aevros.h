#ifndef AEVROS_H
#define AEVROS_H

#define AEV_READ_ONLY 0x01
#define AEV_WRITE_ONLY 0x02
#define AEV_READ_WRITE 0x03
#define AEV_CREATE 0x04

#define AEV_STDIN 0
#define AEV_STDOUT 1

int write(int fd, const char *buf, int size);

int read(int fd, char *buf, int size);

int open(const char *path, int flags);

int close(int fd);

void exit(int code);

int strlen(const char *s);

void puts(const char *s);

void putnum(int n);

int main(void);

#endif