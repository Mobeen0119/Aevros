# Contributing to Aevros

Thank you for your interest in contributing to Aevros.

Aevros is an operating system built from scratch to explore how operating systems work and, more importantly, to make those internals understandable. The project values clean design, readable code, and built-in observability over simply adding more features.

Whether you're fixing a bug, improving documentation, or implementing a new subsystem, your contribution should help make the kernel easier to understand.

## Before You Begin

Before making changes, please read:

- `docs/PHILOSOPHY.md`
- `docs/ARCHITECTURE.md`

Understanding the project's design goals will make it much easier to contribute consistently.

## What Makes a Good Contribution?

Aevros is not a project whose goal is to collect as many operating system features as possible.

Before implementing something, ask:

- Why does this feature belong in Aevros?
- What problem does it solve?
- Will it help someone understand how an operating system works?
- Can the kernel explain what the feature is doing while it is running?

For example, adding networking or a GUI simply because "every OS has one" is usually not a good reason.

A stronger contribution would be:

- making an existing subsystem easier to understand
- improving kernel diagnostics
- exposing hidden kernel state through new shell commands
- simplifying an existing implementation
- improving documentation
- adding self-tests
- helping the kernel explain *why* something happened, not only *what* happened

A feature is valuable when it improves understanding, not just when it increases the feature count.

## Development Setup

See `docs/BUILDING.md` for the complete build environment.

Quick start:

```bash
git clone https://github.com/Mobeen0119/Aevros.git
cd Aevros
./build.sh
qemu-system-i386 -cdrom aevrosos.iso
```

## Coding Guidelines

### Keep code readable

Write code that is easy to understand. Prefer clear logic over clever shortcuts.

### Match the existing style

Follow the formatting, naming, and structure already used in the file you are modifying.

### Build from first principles

Aevros is written from scratch for learning and experimentation.

Avoid copying implementations directly from other operating systems. Understanding why something works is more important than reproducing an existing design.

### Observability matters

Whenever practical, new subsystems should expose useful diagnostic information.

If a resource can exist, it should be possible to understand:

- who owns it
- why it exists
- when it was created
- what happens if it disappears

This philosophy is what makes Aevros different.

### Add tests

Whenever possible, accompany new functionality with a self-test.

Update `kernel/selftest.c` if your feature can be tested automatically.

### Update documentation

Documentation should always reflect the current implementation.

If your pull request changes behavior, update the relevant documentation in the same pull request.

## Pull Requests

Please keep pull requests focused.

A pull request should:

- solve one problem
- avoid unrelated formatting changes
- include a clear description of why the change is needed
- pass existing self-tests
- update documentation when necessary

Small, focused pull requests are much easier to review than large mixed changes.

## Commit Messages

Describe **why** the change was made, not only **what** changed.

Good:

```
Fix page fault handler when writing to unmapped memory
```

Less helpful:

```
Fix page fault
```

## Reporting Bugs

When reporting a bug, include:

- what happened
- what you expected
- steps to reproduce
- shell commands used
- panic output or logs if available
- screenshots if helpful

Reports that include enough information to reproduce the problem are much easier to investigate.

## Security

Please do not report security vulnerabilities through public GitHub issues.

See `SECURITY.md` for the preferred reporting process.

## Code of Conduct

By participating in this project, you agree to follow the guidelines described in `CODE_OF_CONDUCT.md`.

## Questions

If you are unsure about an idea, implementation, or subsystem, open an issue before writing a large amount of code.

Discussion is encouraged, and asking questions early usually saves everyone time.