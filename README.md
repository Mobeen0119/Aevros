<div align="center">

# Aevros

**An operating system that explains itself.**

[![C](https://img.shields.io/badge/language-C-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Architecture](https://img.shields.io/badge/arch-x86--32-lightgrey.svg)](#)
[![Status](https://img.shields.io/badge/status-active--development-orange.svg)](#)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

[Philosophy](docs/PHILOSOPHY.md) • [Architecture](docs/ARCHITECTURE.md) • [Shell Commands](docs/COMMANDS.md) • [Building](docs/BUILDING.md) • [Contributing](CONTRIBUTING.md)

</div>

---

## What is Aevros

Aevros is a small operating system kernel, written from scratch in C and x86 assembly, that you can boot in a few minutes and start asking questions.

Most teaching operating systems show you *what* is happening. Aevros tries to go one step further and tell you *why*. It has its own memory manager, its own process scheduler, its own filesystem, and its own shell, and every one of those subsystems was built by hand, without borrowing another kernel's code or design.

What makes it different from the usual "toy kernel" project is a small set of built-in tools that let the running system inspect and explain its own state, in plain sentences, from inside the shell. Kill a process and ask the kernel what would have broken. Find a suspicious allocation and ask the kernel who owns it and whether that owner is still alive. Trigger a page fault and get a human-readable paragraph back instead of a raw hex dump.

That idea, a kernel that can answer "why" about itself, is the whole point of this project. It is explained in full in [`docs/PHILOSOPHY.md`](docs/PHILOSOPHY.md).

## A quick taste

```text
Aevros > exec /forktest.elf
[kernel] pid 4 (forktest) started

Aevros > blast 4

  BLAST RADIUS
  if pid 4 (forktest) disappears:

  CHILDREN (would become orphans):
    (none)

  FILES HELD:
    /forktest.elf    ref_count=1, would drop to 0 and be reclaimed

  IMPACT : LOW
    0 orphaned child(ren), 1 file(s) would free, 0 file(s) remain safely shared

Aevros > whyalive alloc 0x2c1000

  WHYALIVE :: allocation
  OBJECT  : heap allocation (64 bytes)
  ORIGIN  : kernel_Process_task.c:118 in task_create_kernel() by pid 1
  WHY     : owning pid 1 is still alive
  VERDICT : [OK]     allocation is in active use
```

No debugger attached. No print statements added. The kernel is just answering the question directly.

## Core ideas

- **Nothing borrowed.** The physical memory manager, the buddy and slab allocators, the paging code, the scheduler, the virtual filesystem, the ELF loader, and the shell are all original code, written to understand how each piece actually works rather than to copy an existing design.
- **Deterministic and inspectable.** State transitions are logged. Allocations are tracked back to a file, a line, and an owning process. Nothing changes silently.
- **The kernel explains its own failures.** A page fault does not just print a faulting address, it tells you in a sentence what kind of access failed and what that usually means. See [`decode_fault`](kernel/Paging/Aevros_Panic/aevros_panic.c) and the panic screen it feeds.
- **Built for reading, not just running.** The codebase is organized so a subsystem and the tool that explains it live side by side (for example `kernel/Process/Blast` sits right next to `kernel/Process/task.c`).

## What's implemented

| Area | Status | Notes |
|---|---|---|
| Boot (Multiboot, GRUB) | Working | `boot/boot.s`, boots under QEMU |
| GDT / IDT / PIC / interrupts | Working | Flat segmentation, remapped PIC, ISR/IRQ dispatch |
| Physical memory manager (PMM) | Working | Bitmap-based frame allocator |
| Paging | Working | Page directories/tables, page fault decoding |
| Buddy allocator | Working | Power-of-two physical block allocator |
| Slab allocator | Working | Fixed-size object caching on top of the buddy allocator |
| Kernel heap (`kmalloc`/`kfree`) | Working | Backed by the slab/buddy layers |
| Allocation tracker | Working | Every live allocation is tied to a file, line, function, and pid |
| Tasking / scheduler | Working (cooperative) | Ready queue, context switching, not yet preemptive by default |
| Fork / Exec | Working | `fork()` clones a task, `exec()` loads and runs an ELF binary |
| ELF loader | Working | Loads flat ELF binaries built for the `User/` toolchain |
| Syscalls | Working | `int 0x80` gate: write, read, open, close, fork, exit, waitpid, exec |
| Virtual filesystem (VFS) | Working | Unified dentry/inode tree over pluggable backends |
| RAMFS | Working | In-memory filesystem, the default root |
| DevFS | Experimental | Device node registration, inode resolution still being hardened |
| Shell | Working | Built-in commands, tokenizer/parser, colored output |
| Checkpoint / restore (`AevrosPoint`) | Working | Save and restore a task's full state under a name |
| Task lifetime log (`TaskLife`) | Working | Every task keeps an event history: created, forked, exited, quarantined, ... |
| Liveness inspector (`WhyAlive`) | Working | Explains why an inode, task, or allocation is still alive |
| Leak scanner (`FDLeak`) | Working | Finds file descriptors that were opened and never closed |
| Blast radius (`Blast`) | Working | Simulates the effect of killing a task before you kill it |
| Quarantine | Working | Automatically freezes (not kills) tasks that look like they're leaking |
| Memory snapshots (`MemFreeze`) | Working | Snapshot the allocator state and diff it later |
| Self-test suite (`selftest`) | Working | Boots straight into a set of in-kernel sanity checks |
| User space / ring 3 | Partial | Programs run through `exec`, full ring 3 isolation still maturing |

Every item marked "Working" above ships with an automated in-kernel self-test (`kernel/selftest.c`) that runs on demand from the shell (`selftest`) or in CI. Nothing in this table is aspirational.

## Try it

```bash
git clone https://github.com/Mobeen0119/Aevros.git
cd Aevros
./build.sh
qemu-system-i386 -cdrom aevrosos.iso
```

Full requirements, dependency install commands per platform, and a walkthrough of what you'll see on first boot are in [`docs/BUILDING.md`](docs/BUILDING.md).

## Documentation map

| Document | What's in it |
|---|---|
| [`docs/PHILOSOPHY.md`](docs/PHILOSOPHY.md) | Why Aevros exists, and what "a kernel that explains itself" actually means in practice |
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Full system design, subsystem by subsystem, with diagrams |
| [`docs/COMMANDS.md`](docs/COMMANDS.md) | Every shell command, explained through a real example session |
| [`docs/BUILDING.md`](docs/BUILDING.md) | Toolchain setup, build steps, running under QEMU, troubleshooting |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | How to propose changes, coding conventions, PR process |
| [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md) | Expected behavior in the project's spaces |
| [`SECURITY.md`](SECURITY.md) | How to report a vulnerability |

## Project layout

```text
Aevros/
├── boot/            Multiboot entry point (assembly) and GRUB config
├── kernel/
│   ├── CPU/         GDT, IDT, TSS
│   ├── Memory/      PMM, paging, buddy allocator, slab allocator, heap,
│   │                allocation tracker, memory snapshots
│   ├── Paging/      Page fault handling and the self-explaining panic screen
│   ├── Process/     Scheduler, tasks, fork/exec, and the introspection
│   │                tools (WhyAlive, Blast, Quarantine, TaskLife, ...)
│   ├── Syscall/     The int 0x80 syscall gate and dispatcher
│   ├── VFS/         Virtual filesystem tree, RAMFS
│   ├── Dev/         Device filesystem (experimental)
│   ├── ELF/         ELF binary loader
│   └── Shell/       The interactive shell and its built-in commands
├── Drivers/         Keyboard, TTY, PIT (timer)
├── Lib/             Freestanding string/printf/math helpers
├── Include/         Shared public headers
├── User/            Userspace test programs and their linker scripts
└── build.sh         One-command build: assembles, compiles, links, makes the ISO
```

## Status and stability

Aevros is under active development. Core subsystems are rewritten as understanding improves, not left alone once they "work". If you're evaluating it for anything beyond learning and experimentation: don't, yet. That's not modesty, it's the actual state of a project without full ring 3 isolation and a preemptive scheduler.

## License

Aevros is released under the [MIT License](LICENSE). You can use, modify, and redistribute it, including commercially, as long as the license and copyright notice are kept. See [`LICENSE`](LICENSE) for the full text.

## Contributing

Contributions are welcome. Start with [`CONTRIBUTING.md`](CONTRIBUTING.md) for setup and conventions, and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) to get oriented before touching a subsystem. Good first issues are labeled `good-first-issue` in the issue tracker.
