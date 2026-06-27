# ForgeOS

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