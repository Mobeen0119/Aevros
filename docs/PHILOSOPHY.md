# Philosophy

Most operating systems tell you **what** happened.

Aevros tries to explain **why**.

When memory leaks, a process refuses to die, or a page fault occurs, most kernels leave the investigation to you. You read logs, add debug prints, reboot, and slowly piece together the answer.

Aevros takes a different approach.

Instead of only managing resources, it records enough information to explain them. Every subsystem is expected to know what it owns, why it exists, and what would happen if it disappeared.

That idea appears throughout the kernel.

- `WhyAlive` explains why a task, allocation, or inode still exists.
- `Blast` predicts what happens before killing a process.
- `TaskLife` records the story of every process.
- `MemFreeze` shows what changed between snapshots.
- `FDLeak` finds forgotten file descriptors.
- `Quarantine` freezes suspicious tasks instead of silently destroying them.
- `Aevros Panic` explains crashes in human language.

These are not debugging tools added later. They are part of the operating system's design.

## For contributors

When adding a feature, don't stop after making it work.

Ask yourself:

- Can someone understand this while the system is running?
- Can the kernel explain why this resource exists?
- Can it explain who owns it?
- Can it explain what happens if it disappears?

If the answer is no, the feature probably isn't finished.

Every subsystem should leave behind enough information to explain itself.

That's the philosophy behind Aevros.

Not just an operating system.

An operating system that explains itself.