#!/usr/bin/env bash

ISO="${1:-aevrosos.iso}"
T=$(mktemp -d)
QPID=""
trap '[ -n "$QPID" ] && kill $QPID 2>/dev/null; rm -rf "$T"' EXIT

command -v qemu-system-i386 >/dev/null || { echo "qemu-system-i386 not found (install your distro's qemu-system-x86 package)"; exit 2; }
command -v python3 >/dev/null || { echo "python3 not found"; exit 2; }
[ -f "$ISO" ] || { echo "no ISO at $ISO (run ./build.sh first)"; exit 2; }

PORT=$(python3 -c 'import socket; s = socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')

qemu-system-i386 -cdrom "$ISO" -m 256 -display none -vga std \
  -netdev user,id=n0,hostfwd=tcp:127.0.0.1:$PORT-:7007 -device rtl8139,netdev=n0 \
  -serial file:"$T/serial" -debugcon file:"$T/debug" \
  -monitor unix:"$T/mon",server,nowait -no-reboot >"$T/qemu.log" 2>&1 &
QPID=$!

# loaded machine can take a few seconds to create the log files
for i in $(seq 20); do
  [ -e "$T/serial" ] && break
  kill -0 $QPID 2>/dev/null || break
  sleep 1
done
if ! kill -0 $QPID 2>/dev/null || [ ! -e "$T/serial" ]; then
  echo "QEMU did not start. Its error output:"
  cat "$T/qemu.log"
  echo "(try the same flags by hand: -netdev user needs a QEMU built with slirp, -device rtl8139 needs the rtl8139 device)"
  exit 2
fi

for i in $(seq 45); do grep -q 'bound:' "$T/serial" 2>/dev/null && break; sleep 1; done
sleep 2

BEFORE=$(wc -c < "$T/serial")
python3 - "$T/mon" <<'PY'
import socket, sys, time
s = socket.socket(socket.AF_UNIX); s.connect(sys.argv[1]); time.sleep(.3)
s.send(b"sendkey esc\n"); time.sleep(2)
for ch in "provenance\n":
    s.send(("sendkey " + {"\n": "ret"}.get(ch, ch) + "\n").encode()); time.sleep(.12)
time.sleep(.5)
PY
sleep 3
tail -c +$((BEFORE + 1)) "$T/serial" > "$T/prov"

python3 - "$T/mon" <<'PY'
import socket, sys, time
s = socket.socket(socket.AF_UNIX); s.connect(sys.argv[1]); time.sleep(.3)
for ch in "tcpecho\n":
    s.send(("sendkey " + {"\n": "ret"}.get(ch, ch) + "\n").encode()); time.sleep(.12)
time.sleep(.5)
PY
sleep 4
python3 - "$PORT" > "$T/tcp" <<'PY'
import socket, sys, time
port, ok, N = int(sys.argv[1]), 0, 20
for i in range(N):
    try:
        c = socket.create_connection(("127.0.0.1", port), timeout=10)
        msg = (b"connection %d " % i) * 8
        c.sendall(msg)
        got = b""
        while len(got) < len(msg):
            chunk = c.recv(256)
            if not chunk:
                break
            got += chunk
        c.close()
        ok += got == msg
    except OSError:
        pass
    time.sleep(0.8)
print("%d of %d" % (ok, N))
PY
sleep 3

fails=0
check() { if eval "$2"; then echo "PASS: $1"; else echo "FAIL: $1"; fails=$((fails + 1)); fi; }
check "kernel is still running"                       'kill -0 $QPID 2>/dev/null'
check "no fault (debug port empty)"                   '[ -e "$T/debug" ] && [ ! -s "$T/debug" ]'
check "kernel printed boot output"                    '[ -s "$T/serial" ]'
check "IPv6 router advertisement received"            'grep -q "Router Advertisement" "$T/serial"'
check "DHCP lease bound"                              'grep -q "bound:" "$T/serial"'
check "no rejected DHCP replies"                      '[ -s "$T/serial" ] && ! grep -q "ignoring a DHCP reply" "$T/serial"'
check "provenance shows received packets (DELIVERY)"  'grep -q "DELIVERY" "$T/prov"'
check "provenance shows sent packets (SYSTEM_ACTION)" 'grep -q "SYSTEM_ACTION" "$T/prov"'
check "TCP: 20 connections echoed intact through the NIC" 'grep -q "^20 of 20" "$T/tcp"'
check "TCP: every connection slot was released"      '[ "$(grep -c "letting go of the slot" "$T/serial")" -ge 20 ]'

[ $fails -eq 0 ] && echo "BOOT CHECK PASSED" || { echo "BOOT CHECK FAILED ($fails)"; echo "--- last serial lines:"; tail -12 "$T/serial" | tr -c '[:print:]\n' '?'; }
exit $fails
