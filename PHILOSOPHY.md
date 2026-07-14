# Philosophy: a kernel that explains itself

## The problem this project is reacting to

Most operating systems, including the small teaching kernels people build to learn from, are built to be *fast* and *correct*. Almost none of them are built to be *understood while they're running*.

When something goes wrong in a typical hobby kernel, the honest debugging loop looks like this: add a `printf`, rebuild, reboot, reproduce, read a hex address, cross-reference it against a symbol table by hand, guess, repeat. You end up learning the kernel by fighting it.

Aevros starts from a different question: what if the kernel already knew the answer to "why", and you could just ask it?

## What "explains itself" actually means

It does not mean a chatbot bolted onto a kernel. It means something much more mechanical and much more useful: every subsystem that manages a resource also keeps enough of a paper trail to answer a direct question about that resource, on demand, in a sentence a human can read without a debugger.

Concretely, that shows up as three habits that run through the whole codebase:

**1. State changes are logged, not just applied.**
When a task is created, forked, given a file descriptor, quarantined, or killed, that event is appended to the task's own event log (`kernel/Process/TaskLife`). The scheduler doesn't just flip a state flag and move on. It leaves a trail that can be replayed later with `tasklife <pid>`.

**2. Ownership is tracked, not assumed.**
Every heap allocation is recorded with the file, line, and function that made it, and the pid that owns it (`kernel/Memory/KallocTracker`). Every open file descriptor is tied to the inode it points at and the task holding it. This is what makes it possible to ask "who is holding this" instead of just "how much memory is used".

**3. Failures are decoded, not just reported.**
When a page fault happens, the kernel doesn't stop at printing the faulting address and the CPU error code. `decode_fault()` in `kernel/Paging/Aevros_Panic/aevros_panic.c` reads the same bits a human would and turns them into an actual sentence: *"tried to write to memory that's mapped read-only"*, followed by a verdict like *"this is a classic NULL pointer bug"* when the address is near zero. The panic screen then walks the stack frame by frame and prints "how the kernel got here" as a small narrated call chain, not a raw address dump.

## Tools built on those three habits

These aren't separate features bolted on top. They're small, focused programs that read the paper trail above and turn it into an answer:

- **`whyalive`** answers "why does this still exist". Point it at an inode, a task, or a raw allocation and it cross-references live holders against the recorded reference count, then gives a verdict: `OK`, `LEAK`, `MISMATCH`, or `GHOST`.
- **`blast`** answers "what would happen if I killed this" *before* you kill it. It walks the process's children, its open files, and works out which files would actually be freed versus which are shared and safe.
- **`quarantine`** is the same idea running automatically in the background. Instead of a human asking "is this task misbehaving", the kernel watches file descriptor churn and freezes (never silently kills) a task that crosses a threshold, so it can be inspected exactly as it was at the moment something looked wrong.
- **`memfreeze`** answers "what changed" by snapshotting allocator state and diffing it against a later snapshot.
- **`fdleak`** and **`outlook`** sweep the whole system looking for the same kind of "this doesn't add up" pattern that `whyalive` checks for one object at a time.
- **`aevros_panic`** is the same philosophy applied to the worst case: the kernel is about to die, and it still spends its last moments trying to tell you why in plain language instead of just halting.

If you're implementing a new subsystem and it manages a resource (memory, a task, a file, a device, anything with a lifetime) the expectation is that it should be able to answer "why do you still exist" and "what happens if I remove you" the same way. That's the bar, not a stretch goal.

## Why this matters for a project meant to be read

Aevros isn't trying to be a production OS. Its purpose, stated directly in the README, is deep systems understanding: build it yourself, break it, and see why it broke. A kernel that can only tell you *that* something went wrong teaches you to fear the failure. A kernel that tells you *why* teaches you the mechanism.

That's the whole philosophy in one line: **every subsystem should be able to explain itself, in a sentence, to the person who just broke it.**

## What this philosophy is not

- It is not a promise that Aevros is bug-free. The introspection tools exist *because* bugs happen, not instead of them.
- It is not artificial intelligence or heuristics. Every explanation above is deterministic: the same state always produces the same verdict, because it's built from the same recorded facts every time, not a guess.
- It is not a substitute for good design. Tracking a leak after the fact with `whyalive` is a debugging aid, not a reason to skip writing correct cleanup paths.

## Applying it as a contributor

When you add or touch a subsystem, ask yourself the two questions this project is built around:

1. If someone asks the kernel "why does this still exist", can it answer, or does it have to guess?
2. If someone is about to remove this, can the kernel tell them what will break, or will they just find out?

If the answer to either is "it can't", that's usually a sign the subsystem needs a small introspection hook before it's done, not after.

See [`docs/ARCHITECTURE.md`](ARCHITECTURE.md) for how these ideas map onto the actual subsystems, and [`docs/COMMANDS.md`](COMMANDS.md) to see every one of these tools used in a real shell session.
