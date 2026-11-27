# Writing programs for Aevros

Programs are written in C, built on your computer, downloaded into Aevros with
`fetch`, and started with `exec`.

## Build

You need `gcc` with 32-bit support and `ld`:

    sudo apt install gcc-multilib binutils

Put ur program in `User/Apps/<name>.c` and run:

    cd User && ./build.sh

This creates `User/out/<name>.elf` for every program in `User/Apps/`.

## Write

A program defines `int main(void)` and returns an exit code. The library in
`User/lib/aevros.h` gives you:

- `write(fd, buf, size)`, `read(fd, buf, size)`, `open(path, flags)`, `close(fd)`
- `puts(string)`, `putnum(number)`, `strlen(string)`
- `exit(code)`

Look at `User/Apps/hello.c` and `User/Apps/cat.c` for examples.

## Run

Serve the `.elf` file from your computer, for example:

    cd User/out && python3 -m http.server 8000 --bind 127.0.0.1

In Aevros (press ESC first to reach the shell):

    fetch /hello.elf /hello.elf
    exec /hello.elf

`fetch` saves the file in memory, so it is gone after a reboot.

## Limits

- No arguments are passed to programs yet.
- Only the file calls above are wrapped. Sockets and processes are not in the library.
- No C standard library: there is no `printf`, `malloc`, or `string.h`.
- Programs are built for 32-bit x86 and run in user mode, but the isolation is not
  yet complete, so treat them as experiments.
