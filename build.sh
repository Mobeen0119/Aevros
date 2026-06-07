#!/bin/bash

rm -f *.o kernel.elf forgeos.iso

# Assemble files
nasm -f elf32 ./kernel/CPU/idt.asm -o idt_asm.o
# ... (include your other asm compilation lines here if you have them)

# 3. Compile ALL KERNEL C source files safely (Ignoring User Programs)
for c_file in $(find . -name "*.c" | grep -v "buddy.c" | grep -v "slab.c" | grep -v "iso/" | grep -v "./User/"); do
    echo "Compiling Kernel File: $c_file"
    gcc -m32 -ffreestanding -c "$c_file" -o "$(basename "${c_file%.c}.o")"
done

# Link every generated object file
echo "Linking architecture files into kernel.elf..."
ld -m elf_i386 -T linker.ld -o kernel.elf *.o

# Build your bootable operating system ISO image
if [ -f kernel.elf ]; then
    mkdir -p iso/boot/grub
    cp kernel.elf iso/boot/
    grub-mkrescue -o forgeos.iso iso/
    echo " ForgeOS successfully forged into forgeos.iso!"
else
    echo "❌ Linking failed. Check missing references above."
fi

