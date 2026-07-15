# Philosophy: a kernel that explains itself

## The problem

Most kernels, including the small teaching ones people build to learn from, are built to be fast and correct. Almost none of them are built to be understood while they're running.

When something breaks in a typical hobby kernel, here's the loop: add a `printf`, rebuild, reboot, reproduce, read a hex address, cross-reference it against a symbol table by hand, guess, repeat. You end up learning the kernel by fighting it, one crash at a time.

Aevros starts from a different question: what if the kernel already knew the answer to "why", and you could just ask it?

## What "explains itself" actually means

Not a chatbot bolted onto a kernel. Something much more mechanical: every subsystem that manages a resource keeps enough of a paper trail to answer a direct question about that resource, on demand, in a sentence you can read without a debugger.

Three habits run through the whole codebase to make that possible:

**State changes get logged, not just applied.**
When a task is created, forked, given a file descriptor, quarantined, or killed, that event gets appended to the task's own event log (`kernel/Process/TaskLife`). The scheduler doesn't flip a state flag and move on, it leaves a trail you can replay later with `tasklife <pid>`.

**Ownership is tracked, not assumed.**
Every heap allocation is recorded with the file, line, and function that made it, and the pid that owns it (`kernel/Memory/KallocTracker`). Every open file descriptor is tied to the inode it points at and the task holding it. This is what makes "who's holding this" an answerable question instead of a guess.

**Failures get decoded, not just reported.**
A page fault doesn't stop at printing the faulting address and the CPU error code. `decode_fault()` in `kernel/Paging/Aevros_Panic/aevros_panic.c` reads the same bits a human would and turns them into an actual sentence: *"tried to write to memory that's mapped read-only"*, or *"this is a classic NULL pointer bug"* when the address is near zero. The panic screen then walks the stack frame by frame and prints "how the kernel got here" as a narrated call chain, not a raw address dump.

## The tools built on those habits

These aren't separate features bolted on top, they're small, focused programs that read the paper trail above and turn it into an answer:

- **`whyalive`** answers "why does this still exist." Point it at an inode, a task, or a raw allocation, it cross-references live holders against the recorded reference count and gives a verdict: `OK`, `LEAK`, `MISMATCH`, or `GHOST`.
- **`blast`** answers "what happens if I kill this" before you kill it. It walks the process's children and open files and works out which files would actually get freed versus which are shared and safe.
- **`quarantine`** runs the same idea automatically. Instead of a human asking "is this task misbehaving," the kernel watches file descriptor churn and freezes (never silently kills) a task that crosses a threshold, so you can inspect it exactly as it was when something looked wrong.
- **`memfreeze`** answers "what changed" by snapshotting allocator state and diffing it later.
- **`fdleak`** and **`outlook`** sweep the whole system for the same kind of "this doesn't add up" pattern `whyalive` checks one object at a time.
- **`aevros_panic`** applies the same philosophy to the worst case. The kernel's about to die, and it still spends its last moments trying to tell you why in plain language instead of just halting.

If you're building a new subsystem and it manages a resource, memory, a task, a file, anything with a lifetime, it should be able to answer "why do you still exist" and "what breaks if I remove you" the same way. That's the bar, not a stretch goal.

## Why this matters here specifically

Aevros isn't trying to be a production OS. Its purpose is stated plainly in the README: deep systems understanding, build it yourself, break it, see why it broke. A kernel that only tells you *that* something went wrong teaches you to fear the failure. A kernel that tells you *why* teaches you the mechanism.

That's the philosophy in one line: every subsystem should be able to explain itself, in a sentence, to the person who just broke it.

## What this philosophy isn't

- Not a claim that Aevros is bug-free. The introspection tools exist because bugs happen, not instead of them.
- Not AI, not heuristics. Every verdict above is deterministic, same state, same answer, every time, built from recorded facts, not a guess.
- Not a substitute for good design. Catching a leak after the fact with `whyalive` is a debugging aid, not a reason to skip writing a correct cleanup path.

## Applying it as a contributor

Two questions, ask them about anything you're building:

1. If someone asks the kernel "why does this still exist," can it answer, or does it have to guess?
2. If someone's about to remove this, can the kernel tell them what will break, or will they just find out?

If either answer is "it can't," that's usually a sign the subsystem needs a small introspection hook before it's done, not after.

See [`docs/ARCHITECTURE.md`](ARCHITECTURE.md) for how this maps onto the actual subsystems, and [`docs/COMMANDS.md`](COMMANDS.md) to watch every one of these tools running for real, with screenshots.