#!/usr/bin/env bash
set -euo pipefail
shopt -s nullglob

BUILD_DIR="build_tmp"
BOOT_SRC="./boot/boot.s"

echo "🧹 Cleaning workspace..."
rm -f boot.o kernel.elf forgeos.iso
rm -rf "$BUILD_DIR" iso/

mkdir -p "$BUILD_DIR"

echo "🔨 Forging ForgeOS..."

if ! command -v nasm >/dev/null 2>&1; then
    echo "nasm is required for assembly. Install nasm and rerun."
    exit 1
fi

if [ ! -f "$BOOT_SRC" ]; then
    echo "❌ $BOOT_SRC not found."
    exit 1
fi

echo "Assembling boot loader..."
nasm -f elf32 "$BOOT_SRC" -o boot.o


asm_objects=()
while IFS= read -r -d '' asm_file; do
    obj_file="$BUILD_DIR/$(printf '%s' "${asm_file#./}" | sed 's#/#_#g; s#\.[^.]*$#.asm.o#')"
    echo "Assembling $asm_file -> $obj_file"
    nasm -f elf32 "$asm_file" -o "$obj_file"
    asm_objects+=("$obj_file")
done < <(find . -path "./$BUILD_DIR" -prune \
    -o -path "./User" -prune \
    -o \( -name "*.asm" -o -name "*.s" \) ! -name "boot.s" -print0)
    

c_objects=()
while IFS= read -r -d '' c_file; do
    obj_file="$BUILD_DIR/$(printf '%s' "${c_file#./}" | sed 's#/#_#g; s#\.c$#.c.o#')"
    echo "Compiling $c_file -> $obj_file"
    gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -nostdlib -c "$c_file" -o "$obj_file"
    c_objects+=("$obj_file")
done < <(find . -type f -name "*.c" \
    ! -path "./User/*" \
    ! -path "./iso/*" \
    ! -path "./$BUILD_DIR/*" \
    -print0)

if [ ${#c_objects[@]} -eq 0 ] || [ ${#asm_objects[@]} -eq 0 ]; then
    echo "❌ No source files were compiled. Check the repository layout."
    exit 1
fi

echo "Linking kernel.elf..."
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
    if command -v grub-mkrescue >/dev/null 2>&1; then
        grub-mkrescue -o forgeos.iso iso/
        echo "✨ ForgeOS successfully forged into forgeos.iso!"
    else
        echo "⚠️  grub-mkrescue not found; kernel.elf was produced but ISO was not generated."
    fi
else
    echo "❌ Linking failed. Check the symbol errors above."
    exit 1
fi