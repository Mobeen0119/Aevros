#!/usr/bin/env bash
# Boots in QEMU with a virtual RTL8139, waits for DHCP  saves the screen
set -u

ISO=${1:?usage: $0 <aevrosos.iso> <out.ppm>}
OUT=$(realpath -m "${2:?usage: $0 <aevrosos.iso> <out.ppm>}")

command -v qemu-system-i386 >/dev/null || { echo "qemu-system-i386 not found (install qemu-system-x86)"; exit 2; }
command -v python3 >/dev/null || { echo "python3 not found"; exit 2; }
[ -f "$ISO" ] || { echo "no ISO at $ISO (run ./build.sh first)"; exit 2; }

T=$(mktemp -d)
PORT=$(python3 -c 'import socket; s = socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')

qemu-system-i386 -cdrom "$ISO" -m 256 -display none -vga std \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:$PORT-:7007 -device rtl8139,netdev=n0 \
  -serial file:"$T/serial" -monitor unix:"$T/mon",server,nowait -no-reboot >"$T/qemu.log" 2>&1 &
QPID=$!

for _ in $(seq 40); do
  grep -q 'bound:' "$T/serial" 2>/dev/null && break
  sleep 1
done
sleep 4

python3 - "$T/mon" "$OUT" <<'PY'
import socket, sys, time
s = socket.socket(socket.AF_UNIX)
s.connect(sys.argv[1])
time.sleep(0.3)
s.send(("screendump %s\n" % sys.argv[2]).encode())
time.sleep(2)
PY

kill $QPID 2>/dev/null
wait $QPID 2>/dev/null

if [ -s "$OUT" ]; then
  echo "saved $OUT"
else
  echo "screendump failed (see $T/qemu.log and $T/serial)"
  exit 1
fi