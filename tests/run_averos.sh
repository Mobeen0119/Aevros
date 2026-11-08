#!/usr/bin/env bash
# Starts Aevros in QEMU.... Usage: ./run_aevros.sh [path/to/aevrosos.iso]
set -e
ISO="${1:-$(dirname "$0")/aevrosos.iso}"
command -v qemu-system-i386 >/dev/null || { echo "Install QEMU first: sudo apt install qemu-system-x86"; exit 1; }
[ -f "$ISO" ] || { echo "No ISO found at $ISO"; exit 1; }

ACCEL=()
if [ -w /dev/kvm ]; then
  ACCEL=(-enable-kvm -cpu host)
  echo "KVM found: running with hardware acceleration."
else
  echo "KVM not available: running in software (slower)."
fi

exec qemu-system-i386 -cdrom "$ISO" -m 256 -vga std "${ACCEL[@]}" \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:5555-:7007 \
  -device rtl8139,netdev=n0