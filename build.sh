#!/bin/bash
set -e

# 1. Clean up ALL previous build artifacts and binaries
echo "🧹 Cleaning workspace..."
rm -f *.o kernel.elf forgeos.iso main.exe kernel.exe
rm -rf iso/

echo "🔨 Forging ForgeOS..."

# 2. Assemble Low-Level CPU & Boot Routines
echo "Assembling assembly source files..."

# Assemble the main bootloader entry point
if [ -f "./boot.s" ]; then
    gcc -m32 -c ./boot.s -o boot.o
fi

# Assemble all NASM-compatible assembly sources except the bootloader.
find . -type f \( -name "*.asm" -o -name "*.s" \) ! -name "boot.s" | while read -r asm_file; do
    obj_file=$(echo "$asm_file" | sed 's#^./##; s#/#_#g; s#\.[^.]*$#.asm.o#')
    echo "Assembling $asm_file -> $obj_file"
    nasm -f elf32 "$asm_file" -o "$obj_file"
done

# 3. Compile ALL Kernel C Source Files Safely
for c_file in $(find . -name "*.c" | grep -v "buddy.c" | grep -v "slab.c" | grep -v "iso/" | grep -v "./User/"); do
    obj_file=$(echo "$c_file" | sed 's#^./##; s#/#_#g; s#\.c$#.c.o#')
    echo "Compiling Kernel File: $c_file -> $obj_file"
    gcc -m32 -ffreestanding -fno-stack-protector -c "$c_file" -o "$obj_file"
done

echo "Linking architecture files into kernel.elf using linker.ld..."
ld -m elf_i386 -T linker.ld -o kernel.elf *.o

if [ -f kernel.elf ]; then
    mkdir -p iso/boot/grub
    cp kernel.elf iso/boot/
    
    cat << EOF > iso/boot/grub/grub.cfg
set timeout=0
set default=0

menuentry "ForgeOS" {
    multiboot /boot/kernel.elf
    boot
}
EOF

    grub-mkrescue -o forgeos.iso iso/
    echo "✨ ForgeOS successfully forged into forgeos.iso!"
else
    echo "❌ Linking failed. Check missing references above."
fi