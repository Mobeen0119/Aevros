# ForgeOS — checkpoint crash: root cause + fixes

I cloned the repo, built it with the included `build.sh`, and ran it in QEMU
(headless, controlled via the monitor + screendumps) to actually reproduce
the bug rather than guess at it.

## What `checkpoint save shell` actually does

`checkpoint save shell` on its own **does not crash**. It always targets
`current_task` (the running shell), and `aevrospoint_save()` skips the
whole page-snapshot path for kernel tasks, so the save itself is harmless.

The crash happens on the **next scheduler tick after `checkpoint restore
shell`** — i.e. as soon as the restored clone gets a chance to run. That's
almost certainly what you hit: save "worked", restore "worked", and then
the machine either silently hung or hard-faulted a moment later, which
looks like "checkpoint save crashes the shell" if you weren't watching
`ps`/registers at the exact moment.

## Your suspicion (`regs = target->regs`)

Not quite that line — struct assignment by value is completely valid C,
`register_t` is a plain `uint32_t` struct, so that copy is fine. But you
were pointing at the right neighborhood: the checkpoint/restore code around
it is where the real bugs are. Three of them, in order of severity:

### Bug 1 (the actual crash): `task->context_esp` was never initialized on restore

The scheduler (`schedule()`/`context_switch()`) resumes a task purely
through `task->context_esp`, which every other task-creation path builds
with `build_initial_stack()`. `aevrospoint_restore()` never called it —
`context_esp` was left at `0` by the `memset()`. The instant the restored
task got scheduled, `context_switch` loaded `ESP = 0`, popped four
registers off address `0`, and returned into a garbage `EIP`. Depending on
timing this either corrupts the kernel silently (interrupts end up
disabled, machine freezes solid — what I saw on the first repro) or faults
outright.

**Fix**: build a proper initial frame with `build_initial_stack()` on
restore, same as `create_process()` does.

### Bug 2: `task->regs` (and therefore the checkpoint) never reflects "where the process really is"

`task->regs.eip` is set exactly once, at task creation, and nothing in the
kernel — no timer interrupt, no syscall — ever updates it again. So a
"snapshot" was never actually a real execution snapshot, just the task's
original entry point. Genuine live-PC resume of an arbitrary running
kernel call stack isn't achievable with this design without a much bigger
rewrite (you'd need every interrupt/syscall path to keep `regs` in sync).
Given that, the honest and safe behavior — and what I implemented — is:
**restore relaunches the task cleanly at its original entry point**, like
a supervised restart rather than a true suspend/resume. It's now
consistent and won't corrupt memory; it's just not literally "freeze mid-
instruction and continue," which the current architecture can't safely do.

### Bug 3: `is_user` detection read a field that's never set

`target->regs.cs` is never populated either (only `regs.eip` is), so
`(target->regs.cs & 0x3)` always evaluated to "kernel," and `cs_saved`/
`ss_saved` were always `0` — which is what produced the general-protection
fault the first time I applied only the Bug 1 fix (`iret` into a null
selector). Fixed to read the real `task_t->is_user` flag and use the
correct selectors (`0x1B`/`0x23` user, `0x08`/`0x10` kernel).

### Bonus: restored tasks were invisible to `ps`

`aevrospoint_restore()` called `task_add_ready()` (scheduler queue) but
never `task_register_all()` (the `all_tasks` list `ps`/`tasklife`/etc.
walk), so a restored task would actually be running but never show up
anywhere. Added the missing registration call.

## Other bugs found and fixed along the way

- **`buddy_init(start, end)` was being called with `(start, size)`.** In
  `main.c`, `buddy_size` was computed as `0x2800000 - buddy_start` (a
  *length*) and then passed into the parameter the function treats as an
  absolute *end address*. This silently shrank the real heap by several
  MB for no reason. Fixed to pass the real end address.
- **`MAX_ORDER` was redefined** in `kheap.c` (`10`, unused) shadowing the
  real one in `buddy.h` (`16`), producing a compiler warning on every
  build. Removed the dead definition.
- **Double prompt on boot / after every command.** `shell_start()` called
  `shell_prompt()` once before the loop *and* again as the first thing
  inside it, so you'd always see `Aevros > Aevros >`. Fixed.

## Shell readability pass

Kept it to real terminal colors, not a rainbow:

- Prompt is now `Aevros` (green) `>` (cyan) on plain black, and resets to
  clean white-on-black before you start typing — no more stray background-color
  bar trailing behind the prompt/typed text.
- `help` is now grouped into sections (general / filesystem / processes /
  memory & diagnostics / testing) with command names in cyan and
  descriptions in a muted grey, so the command and what it does are
  visually distinct instead of one flat wall of text.
- Left the existing green/red/yellow semantics for success/error/usage
  messages as-is — they were already used consistently.

## Verification

Rebuilt and re-ran the exact repro in QEMU after each fix:
`checkpoint save shell` → `checkpoint restore shell` → `ps`. Confirmed via
QEMU's `info registers` (CS=0x0008, CPL=0, halted cleanly waiting on
keyboard input — no fault, no dead interrupt state) and via `ps` itself
showing both `pid 0 shell RUNNING` and `pid 1 shell READY` after a full
save/restore cycle, repeatedly, with no crash or hang.

## Update: the real reason it can still hang on some machines

After the checkpoint fixes above, a hang was still reported on first-ever
`checkpoint save shell`, on a freshly rebuilt ISO. That pointed at
something environment-dependent rather than the checkpoint logic itself
— and there was one: **the kernel never checks how much RAM is actually
present.**

`kernel_main()` hardcoded the heap's end address at `0x2800000` (40MB),
unconditionally. Worse, GRUB hands the kernel a Multiboot info pointer in
`EBX` (with the real detected memory size in it) on every boot, but
`boot.s` never saved it — `EAX`/`EBX` were discarded before `kernel_main`
was even called, so the kernel had no way to know the real number even if
it wanted to.

On a machine/VM with **less than ~40MB of RAM**, the buddy allocator ends
up handing out blocks that don't physically exist. In QEMU (and on real
x86), writes to memory past the end of installed RAM are typically just
silently discarded rather than faulting — so instead of a clean crash you
get quiet corruption. `checkpoint save` is the single largest allocation
anywhere in the kernel (~1MB, growing to ~2MB inside the ramfs write), so
it's the first thing likely to reach past real RAM and freeze the machine
with zero visible error. This fully explains a hang on the very first
`checkpoint save shell`, with nothing printed, screen just stuck — while
my own test VM (which happened to have enough RAM) never showed it.

**Fix:**
- `boot.s` now preserves `EAX`/`EBX` and passes them into `kernel_main(mb_magic, mb_info_addr)`.
- `kernel_main()` reads `multiboot_info->mem_upper` (real KB of RAM above 1MB) and sizes the heap to whatever's actually there, capped at the old 40MB assumption as a ceiling, with a 1MB safety margin below the true top.
- If Multiboot info isn't present for some reason, it falls back to a conservative 8MB heap instead of blindly assuming 40MB.

Re-tested with `-m 24` (24MB RAM, well under the old hardcoded 40MB) —
`checkpoint save shell` now works and the shell stays fully responsive
(`ticks` command answers correctly right after).

If you're still able to reproduce a hang after this, the most useful
things to tell me are: how much RAM you're giving the VM (or if it's
real hardware, how much RAM the machine has), and whether the very same
freeze happens on `ps` or `ls` too (which would point at something
besides the heap-size issue).

## Files changed

- `kernel/Process/AevrosPoint/aevrospoint.c` — the checkpoint/restore fix (context_esp, is_user/cs/ss, all_tasks registration)
- `boot/boot.s` — preserve and pass through the Multiboot magic/info pointer
- `kernel/main.c` — real memory-size detection instead of a hardcoded 40MB heap
- `kernel/Memory/kheap.c` — dead `MAX_ORDER` redefinition removed
- `kernel/Shell/shell.c` — double prompt fix + shell readability pass

`forgeos_fixes.patch` has the full diff; `aevrosos.iso` is the rebuilt,
tested image.
