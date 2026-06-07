#!/bin/bash
set -euo pipefail
shopt -s nullglob

echo "🧹 Cleaning workspace..."
rm -f boot.o *.c.o *.asm.o kernel.elf forgeos.iso main.exe kernel.exe
rm -rf iso/

echo "🔨 Forging ForgeOS..."

if ! command -v nasm >/dev/null 2>&1; then
    echo "❌ nasm is required for assembly. Install nasm and rerun."
    exit 1
fi

# 2. Assemble Low-Level CPU & Boot Routines
echo "Assembling assembly source files..."

if [ -f "./boot.s" ]; then
    nasm -f elf32 ./boot.s -o boot.o
elif [ -f "./boot/boot.s" ]; then
    nasm -f elf32 ./boot/boot.s -o boot.o
else
    echo "❌ boot.s not found."
    exit 1
fi

asm_objects=()
while IFS= read -r asm_file; do
    obj_file=$(echo "$asm_file" | sed 's#^./##; s#/#_#g; s#\.[^.]*$#.asm.o#')
    echo "Assembling $asm_file -> $obj_file"
    nasm -f elf32 "$asm_file" -o "$obj_file"
    asm_objects+=("$obj_file")
done < <(find . -type f \( -name "*.asm" -o -name "*.s" \) ! -name "boot.s")

c_objects=()
while IFS= read -r c_file; do
    obj_file=$(echo "$c_file" | sed 's#^./##; s#/#_#g; s#\.c$#.c.o#')
    echo "Compiling Kernel File: $c_file -> $obj_file"
    gcc -m32 -ffreestanding -fno-stack-protector -c "$c_file" -o "$obj_file"
    c_objects+=("$obj_file")
done < <(find . -name "*.c" | grep -v "buddy.c" | grep -v "slab.c" | grep -v "iso/" | grep -v "./User/")

echo "Linking architecture files into kernel.elf using linker.ld..."
ld -m elf_i386 -T linker.ld -o kernel.elf boot.o "${c_objects[@]}" "${asm_objects[@]}"

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
    echo "❌ Linking failed. Check the symbol errors above."
fi
