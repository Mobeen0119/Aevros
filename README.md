# ForgeOS

<<<<<<< HEAD
A 32-bit x86 hobby kernel with preemptive multitasking, fork/exec, a VFS with
a RAM-backed filesystem, and a small interactive shell.

## Scheduler design

Most small hobby kernels build one assembly routine that tries to handle
three different jobs at once: saving a kernel context, entering user mode
for the first time, and resuming a task that was already running. Mixing
those into one `iret`-shaped switch is what made this kernel's scheduler
fragile - the stack shape needed for "first run" and the stack shape needed
for "resume after a timer tick" are not the same, and a single switch
routine has to track which one it's looking at.

ForgeOS splits this the way xv6 and Linux do: into two small, single-purpose
primitives.

- `context_switch(old_esp, new_esp)` does exactly one thing - save four
  callee-saved registers and the current stack pointer, load the next
  task's stack pointer, restore four registers, `ret`. It never touches
  `cr3`, segment registers, or `iret`. This is the only function that runs
  on every scheduling decision, voluntary or preemptive.

- `trap_return` is the only place in the kernel that executes `iret`. Every
  task - whether it has never run before or is resuming after a timer
  interrupt - reaches user or kernel mode through this single path, popping
  a trapframe that is always built the same way by one shared helper,
  `build_initial_stack`.

Because there's one function that builds a task's initial stack frame and
one function that consumes it, the frame layout can't drift between
`fork()`, `exec()`, kernel-thread creation, and the initial shell task the
way it could when each of those hand-rolled its own copy.

## Process model

- `all_tasks`: a flat list of every task that has ever existed, used by
  `ps` and `waitpid`. Exited tasks become zombies and stay here until a
  parent reaps them.
- `ready_queue`: a ring of currently schedulable tasks. A task is removed
  from this ring the moment it exits - `all_tasks` is what lets `ps` still
  show it afterward.

## Building

```
make -f MakeFile kernel.elf   # build the kernel image
make -f MakeFile iso          # build a bootable ISO via GRUB
qemu-system-i386 -cdrom forgeos.iso
```

## Known limitations

- `sys_exec()` returns control through the normal syscall return path
  rather than forcing an immediate switch, so a replaced process image
  takes effect on the next scheduling decision rather than instantly.

## Fixed: heap allocator header accounting

`kmalloc_raw` previously computed the buddy allocation order from the
requested size alone, without accounting for the block header that gets
prepended to every allocation. Any allocation landing exactly on a
power-of-two boundary - most commonly a 4KB kernel stack - would overflow
by `sizeof(block_header_t)` bytes into the start of the next buddy block,
corrupting its free-list pointer. This surfaced as an intermittent invalid
pointer dereference inside `buddy_alloc`, usually during process creation.
Fixed by deriving the order from `size + sizeof(block_header_t)`. This also
required `buddy_init` to carve memory into a single aligned power-of-two
region rather than several differently-sized top-level blocks, since the
buddy free path's XOR-based address calculation only holds within one
such region.
=======
![C](https://img.shields.io/badge/language-C-blue.svg)
![Status](https://img.shields.io/badge/status-ongoing-orange.svg)
![Architecture](https://img.shields.io/badge/arch-x86-lightgrey.svg)
![Build](https://img.shields.io/badge/build-experimental-red.svg)

> A from-scratch operating system project focused on deeply understanding how kernels actually work — memory, processes, scheduling, and filesystems built manually without abstractions.

---

## ⚙️ Overview

ForgeOS is a low-level operating system project written in **C and x86 assembly**, designed to explore kernel architecture by building every core subsystem from scratch.

This is not a framework-based OS — everything is handcrafted, including memory management, tasking, and filesystem layers.

---

## 🧩 What’s Implemented

### 🧠 Kernel Core
- Basic kernel entry and boot flow
- Modular kernel structure
- Kernel heap allocator (`kmalloc`-style system)
- Assembly + C integration

---

### 🧠 Memory Management
- Physical Memory Manager (PMM)
- Paging system (page directory setup)
- Page fault handler using CR2 analysis
- Basic virtual memory mapping experiments

---

### 🔄 Process & Tasking
- Task structure (`task_t`)
- Context switching hook
- Basic scheduler groundwork (non-preemptive)
- Ready queue system
- PID allocation

---

### 📁 Virtual File System (VFS)
- VFS abstraction layer
- `dentry_t` based filesystem tree
- Unified read/write interface
- FS driver separation design

---

### 💾 RAMFS (In-Memory FS)
- Simple in-memory filesystem
- File registration system
- Basic inode/dentry mapping experiments

---

### 🔌 Device / FS Layer (Experimental)
- Early devfs-style registration logic
- Device-node mapping experiments
- Inode resolution system (work in progress)

---

### 🖥️ Hardware Interaction
- VGA text-mode output system
- Debug logging through screen output
- Direct CPU register interaction (CR3 / CR2 reads)

---

## 🚧 Current Work

- Stabilizing **VFS ↔ RAMFS ↔ DevFS interaction**
- Fixing inode resolution inconsistencies
- Improving task switching stability
- Cleaning memory abstraction layer
- Designing system call interface (early stage)

---

## ⚠️ Known Limitations

- No user-space yet
- Scheduler is not preemptive
- Filesystem layer is still experimental
- Debugging depends heavily on VGA output
- Device filesystem is unstable in edge cases

---

## 🎯 Project Goal

This project is built for **deep systems understanding**, not production use.

Core goals:
- Understand kernel internals by building them
- Avoid high-level abstractions
- Learn by breaking and rebuilding systems
- Gradually evolve toward a minimal functional OS

---

## 🛠️ Tech Stack

- **C** (Kernel implementation)
- **x86 Assembly** (boot & low-level operations)
- Custom memory, process, and filesystem layers

---

## 🗺️ Goals

- [ ] Preemptive multitasking
- [ ] User-mode / ring transitions
- [ ] System call interface
- [ ] ELF binary loader
- [ ] Improved filesystem hierarchy
- [ ] Basic shell / CLI environment

---

## ⭐ Status

> Actively under development  core systems are being continuously rewritten and improved.

---

## 🤝 Contributing

Currently not open for external contributions (early-stage architecture work).

---
>>>>>>> origin/main
