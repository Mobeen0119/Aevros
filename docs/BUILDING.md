# Building and running Aevros

## What you need

Aevros builds a 32-bit ELF kernel from C and NASM assembly, then packs it into a GRUB-bootable ISO. Confirmed working, this exact toolchain built the screenshots in [`docs/COMMANDS.md`](COMMANDS.md). You need:

| Tool | Why | Debian/Ubuntu package |
|---|---|---|
| `nasm` | Assembles the `.s`/`.asm` sources | `nasm` |
| `gcc` (with 32-bit support) | Compiles the C sources with `-m32` | `gcc` + `gcc-multilib` |
| `ld` | Links the final `kernel.elf` | part of `binutils` |
| `grub-mkrescue` | Builds the bootable ISO | `grub-pc-bin` `xorriso` |
| `qemu-system-i386` | Runs the ISO without real hardware | `qemu-system-x86` |

Install everything on Debian/Ubuntu:

```bash
sudo apt update
sudo apt install nasm gcc gcc-multilib binutils grub-pc-bin xorriso qemu-system-x86
```

On Fedora:

```bash
sudo dnf install nasm gcc glibc-devel.i686 binutils grub2-tools grub2-pc-modules xorriso qemu-system-x86
```

On Arch:

```bash
sudo pacman -S nasm gcc binutils grub xorriso qemu-system-x86 lib32-glibc
```

On macOS, cross-compilation is required since Apple's toolchain doesn't target bare-metal ELF directly; the simplest path is building inside a Linux container or VM (Docker, UTM, or similar) rather than trying to get a native i686-elf toolchain working on the host.

## Building

From the repository root:

```bash
./build.sh
```

`build.sh` does the following, in order, and will stop with a clear error if a required tool is missing:

1. Cleans previous build artifacts (`boot.o`, `kernel.elf`, the ISO, `build_tmp/`, `iso/`).
2. Assembles `boot/boot.s` and every other `.s`/`.asm` file in the tree (except files under `build_tmp/` and `User/`) with `nasm -f elf32`.
3. Compiles every `.c` file in the tree (except `User/`, `iso/`, `build_tmp/`) with:
   ```
   gcc -m32 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie \
       -mgeneral-regs-only -mno-sse -mno-sse2 -mno-mmx -mno-80387 \
       -fcf-protection=none -nostdlib -c <file>
   ```
   Those flags matter: this is freestanding kernel code with no host C library, no stack protector canary support (nothing sets one up), and no floating point/SIMD register use, since the kernel never saves that state on a context switch.
4. Links everything against `linker.ld` with `ld -m elf_i386` into `kernel.elf`.
5. Copies `kernel.elf` into `iso/boot/`, writes a matching `iso/boot/grub/grub.cfg`, and runs `grub-mkrescue` to produce `aevrosos.iso`.

If `grub-mkrescue` isn't installed, the script still produces a working `kernel.elf`, it just skips the ISO step and tells you so. You can boot `kernel.elf` directly with QEMU's `-kernel` flag in that case (see below).

## Running

With the ISO (closest to how it would boot on real hardware, via GRUB):

```bash
qemu-system-i386 -cdrom aevrosos.iso
```

Directly from the ELF, skipping GRUB entirely (works even without `grub-mkrescue`):

```bash
qemu-system-i386 -kernel kernel.elf
```

Either way, you should see the VGA text screen clear, then:

```text
                    Welcome to AevrosOS
                Type a command to get started.

Aevros >
```

Try `help`, then `identity` for a full dashboard, then work through [`docs/COMMANDS.md`](COMMANDS.md).

## Rebuilding after a change

`build.sh` always does a clean rebuild, there's no incremental build cache to worry about invalidating. Just rerun it:

```bash
./build.sh && qemu-system-i386 -cdrom aevrosos.iso
```

## Troubleshooting

**`nasm is required for assembly. Install nasm and rerun.`**
`nasm` isn't on your `PATH`. Install it with your package manager (see above) and confirm with `nasm -v`.

**Compiler errors about missing 32-bit headers (e.g. `bits/wordsize.h` not found)**
You have `gcc` but not the 32-bit multilib support. Install `gcc-multilib` (Debian/Ubuntu) or the 32-bit glibc package for your distribution.

**`No source files were compiled. Check the repository layout.`**
`build.sh` is being run from somewhere other than the repository root, or the `kernel/`, `boot/`, `Drivers/`, or `Lib/` directories are missing. Run it from the top of a full checkout.

**Linking fails with undefined reference errors**
Usually means a new `.c` file was added that references a function whose implementation lives in a file the build didn't pick up, most often because it's inside `User/` (excluded on purpose, see [`ARCHITECTURE.md`](ARCHITECTURE.md)) or has a typo in an `#include` path. Check the error against `linker.ld` and the including headers first.

**`grub-mkrescue not found; kernel.elf was produced but ISO was not generated.`**
Not a failure, this is expected if `grub-pc-bin`/`xorriso` aren't installed. Install them if you want the ISO, or just boot with `qemu-system-i386 -kernel kernel.elf` instead.

**QEMU shows a black screen and never reaches the shell prompt**
Usually a boot-time fault before VGA output is set up (very early GDT/IDT/PMM issue). Run under `qemu-system-i386 -cdrom aevrosos.iso -d int -no-reboot -no-shutdown` to get an instruction/interrupt trace on the host terminal, or add `-s -S` to attach GDB at `localhost:1234` and step from the entry point in `boot/boot.s`.

## Committing generated artifacts

Don't commit build output. `.gitignore` already excludes `*.o`, `*.iso`, `*.log`, `kernel.elf`, `iso/`, and `build_tmp/`, keep it that way in pull requests, see [`CONTRIBUTING.md`](../CONTRIBUTING.md).
