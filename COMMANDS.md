# Shell commands, explained through real sessions

This is not a flat command reference. Every command below is shown doing something, with real output shapes taken from the source, because the point of Aevros's shell is that the commands *are* the documentation for the subsystem behind them. If you want the one-line summary of everything, run `help` inside the shell, it's generated from the same list.

Every command lives in `kernel/Shell/shell.c`'s dispatch table. If you add a subsystem and a command for it, add a matching story here in the same PR.

---

## General

### `identity`

Boots straight into a full-screen dashboard: version, uptime in ticks, current pid, task count, and memory summary. It's the "what is this system, right now" command, useful as the first thing you run after boot or after a crash recovery.

### `help`

Prints every command below, grouped the same way this document is grouped. If you're adding a command, add its `help_line(...)` entry in `shell.c` at the same time, the two are expected to stay in sync.

### `clear`

Clears the screen. Nothing more to explain.

---

## Filesystem

Standard, small, and predictable. These all go through the VFS described in [`ARCHITECTURE.md`](ARCHITECTURE.md#5-virtual-filesystem), which is what makes `whyalive inode` and `blast` able to reason about files at all.

```text
Aevros > touch notes.txt
Aevros > write notes.txt "first boot went fine"
Aevros > cat notes.txt
first boot went fine
Aevros > ls
notes.txt
Aevros > tree
/
└── notes.txt
Aevros > rm notes.txt
```

`echo <text> > <file>` redirects into a file the same way `write` does, it's kept as a separate form because it mirrors what people already expect from a shell.

`pwd` and `cd <dir>` track the current task's working directory (`task->cwd`), which is per-task, not global.

---

## Processes

### The story: running something and watching it live

```text
Aevros > exec /forktest.elf
[kernel] pid 4 (forktest) started

Aevros > ps

 ------------------------------------------------------------------------
  PID   ||   NAME       ||   STATE    ||   TICKS ALIVE   ||   PARENT
 ------------------------------------------------------------------------
  1        kernel_idle       RUNNING          812                  0
  4        forktest          READY            6                    1
 ------------------------------------------------------------------------

Aevros > tasklife 4

=== tasklife pid 4 ===
  state      : READY
  created    : tick 806
  alive      : 6 ticks so far
  parent     : pid 1
  events     : 2 / 16
  -----------------------------
   [+0  ticks]  CREATED
   [+1  ticks]  FD_OPEN  fd=3
  user_time  : 0 ticks
  kernel_time: 6 ticks
===================
```

`exec <file>` loads an ELF binary through the VFS and starts it as a new task. `fork` specifically runs the bundled fork demo program, useful for exercising `do_fork` without needing your own test binary. `ps` lists every task Aevros has ever created (`all_tasks`), not just the runnable ones, which is why a `ZOMBIE` task still shows up until something reaps it. `ticks` prints the raw system tick counter `ps` and `tasklife` are timing everything against.

`tasklife <pid>` is the direct read-out of the event log described in [`PHILOSOPHY.md`](PHILOSOPHY.md#1-state-changes-are-logged-not-just-applied): every CREATED, FORKED, FD_OPEN, FD_CLOSE, EXITED, and QUARANTINED event that task has ever had, each stamped with how many ticks after creation it happened.

### `checkpoint save|restore|list <name>`, aka AevrosPoint

This is a manual, named snapshot of one task's complete state, think of it as a game save point for a process.

```text
Aevros > checkpoint save before-crash
[AevrosPoint] saved task pid 4 as 'before-crash'

Aevros > checkpoint list
  before-crash   (pid 4, saved at tick 900)

Aevros > checkpoint restore before-crash
[AevrosPoint] pid 4 restored from 'before-crash'
```

It's useful for reproducing a bug: get a task into the exact state right before something goes wrong, save it, cause the problem, then restore and try a fix without rebooting.

---

## Memory and diagnostics

This is where the "kernel explains itself" idea is most visible. Read [`PHILOSOPHY.md`](PHILOSOPHY.md) first if you haven't, these commands are the direct implementation of that idea.

### `meminfo [pmm|heap|paging|task|buddy|slab]`

A summary view per subsystem. Run with no argument for everything, or narrow it down, e.g. `meminfo buddy` to see just the buddy allocator's free lists.

### `memstory [ghosts | pid <n>]`

Reads the allocation tracker directly. Plain `memstory` lists every live allocation with its size and origin (file:line, function, owning pid). `memstory ghosts` filters to allocations whose owning pid has already exited, the strongest signal of a missing `kfree` on some exit path. `memstory pid <n>` filters to one task.

```text
Aevros > memstory ghosts

  GHOST ALLOCATIONS (owner pid no longer exists)
  -----------------------------------------------
  0x2c1000   64 bytes   kernel_Process_task.c:118   task_create_kernel()   pid 4 (dead)
```

### `whyalive <inode <path> | task <pid> | alloc <addr>>`

The single-object version of the same question `memstory ghosts` answers in bulk. See the full walkthrough in [`PHILOSOPHY.md`](PHILOSOPHY.md#tools-built-on-those-three-habits); the short version:

```text
Aevros > whyalive alloc 0x2c1000

  WHYALIVE :: allocation
  OBJECT  : heap allocation (64 bytes)
  ORIGIN  : kernel_Process_task.c:118 in task_create_kernel() by pid 4
  WHY     : owning pid 4 is dead
  VERDICT : [GHOST]  owner exited but allocation still exists
            memory should have been released
```

`whyalive inode <path>` cross-checks an inode's `ref_count` against the file descriptors actually pointing at it across every task, and reports `OK`, `LEAK` (ref_count says it's held, nobody is holding it), or `MISMATCH` (the numbers don't agree). `whyalive task <pid>` explains why a task is still around, most usefully for a `ZOMBIE`: it names the exact parent pid that hasn't called `waitpid` yet.

### `blast <pid>`

Simulates removing a task *before* you remove it: which children would be orphaned, which open files would actually be freed versus stay safely shared with another task, and an overall LOW/MEDIUM/HIGH impact rating.

```text
Aevros > blast 4

  BLAST RADIUS
  if pid 4 (forktest) disappears:

  CHILDREN (would become orphans):
    (none)

  FILES HELD:
    /forktest.elf    ref_count=1, would drop to 0 and be reclaimed

  IMPACT : LOW
    0 orphaned child(ren), 1 file(s) would free, 0 file(s) remain safely shared
```

### `quarantine [check|release <name>]`

Not something you usually run to start, this is a safety net that runs on its own: if a task opens far more file descriptors than it closes inside a short tick window, the kernel freezes it (state `TASK_QUARANTINED`) instead of letting it run away or killing it outright. `quarantine` with no argument lists what's currently frozen, `quarantine check` forces an immediate sweep, and `quarantine release <name>` puts a task back on the ready queue once you've decided it's fine.

```text
  [KERNEL] pid 7 (leaky) auto-quarantined at tick 240
           reason: fd open rate exceeded threshold (9 opened, 0 closed in 200 ticks)
           task frozen, not killed -- state preserved for inspection

Aevros > quarantine
  QUARANTINE
  ----------
  pid 7    leaky      quarantined

Aevros > quarantine release leaky
quarantine: pid 7 ..... leaky resumed
```

### `memfreeze <snap|diff>`

A before/after tool for memory: `memfreeze snap` records the current allocation tracker table, `memfreeze diff` compares the live table against the last snapshot and shows what was allocated or freed in between. Useful for answering "what did that one command actually cost in memory" by snapping right before it and diffing right after.

### `fdleak`

A system-wide sweep for file descriptors that have been open unusually long relative to how active the owning task is, surfaced per task so you can see which process is the likely source.

### `outlook`

A broader system-wide scan in the same spirit as `fdleak` and `memstory ghosts`, looking across tasks for the same class of "this doesn't add up" signal in one combined view.

### `timeline`

Merges every task's individual event log (the same log `tasklife` reads) into one system-wide, tick-ordered feed. Where `tasklife <pid>` answers "what happened to this task", `timeline` answers "what happened, system-wide, in what order".

### `stackmap <pid>`

Prints how much of a task's allocated stack is actually in use versus untouched, useful for sizing stacks correctly instead of guessing.

### `buddydbg`

A raw debug view of the buddy allocator's free lists, order by order. Lower-level than `meminfo buddy`, meant for working on the allocator itself rather than diagnosing a leak.

---

## Testing

### `selftest`

Runs the in-kernel self-test suite (`kernel/selftest.c`) live, in the booted kernel, not in a separate host-side test runner. It checks the buddy allocator's free lists terminate (no cycles), the heap allocates and survives a write, the ready queue terminates, the shell tokenizer splits arguments correctly, syscall dispatch handles both fd-safety and unknown syscall numbers without crashing, and more. Each check prints `[PASS]` or `[FAIL]` on its own line.

```text
Aevros > selftest
-- buddy --
  [PASS] buddy list terminates
  [PASS] buddy_alloc(0) non-null
  [PASS] buddy list still finite after free
-- heap --
  [PASS] kmalloc_raw(64) non-null
  [PASS] write to allocated block doesn't fault
-- task list --
  [PASS] ready_queue terminates (no cycle)
-- tokenize --
  [PASS] tokenize('ls') argc==1
  [PASS] tokenize('ls') argv[0]=='ls'
  [PASS] tokenize('echo hi there') argc==3
  [PASS] tokenize argv[1]=='hi'
```

If you add a subsystem, add a `test_<subsystem>` function here following the same `CHECK(name, condition)` pattern. See [`CONTRIBUTING.md`](../CONTRIBUTING.md) for the expectation that new features ship with a self-test in the same PR.

`checktest` is a related but separate command: it runs `aevrospoint_full_test()` specifically, exercising checkpoint save/restore under load rather than the general suite.
