# ForgeOS

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
