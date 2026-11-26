#!/usr/bin/env bash
# Builds every program in User/Apps into User/out/<name>.elf
set -e
cd "$(dirname "$0")"
CFLAGS="-m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -mgeneral-regs-only -mno-sse -mno-sse2 -mno-mmx -mno-80387 -fcf-protection=none -nostdlib -O2"
mkdir -p out/obj
gcc $CFLAGS -c lib/crt0.c -o out/obj/crt0.o
gcc $CFLAGS -c lib/aevros.c -o out/obj/aevros.o
for src in Apps/*.c; do
    name=$(basename "$src" .c)
    gcc $CFLAGS -c "$src" -o "out/obj/$name.o"
    ld -m elf_i386 -T lib/program.ld -o "out/$name.elf" out/obj/crt0.o out/obj/aevros.o "out/obj/$name.o"
    echo "built out/$name.elf ($(stat -c %s "out/$name.elf") bytes)"
done