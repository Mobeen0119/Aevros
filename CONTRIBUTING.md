# Contributing to Aevros

Thanks for considering it. Aevros is a from-scratch kernel built for deep systems understanding, and that shapes everything below: a contribution that helps someone understand the system, you included, is worth more here than one that just makes it bigger.

## Before you start

Read [`docs/PHILOSOPHY.md`](docs/PHILOSOPHY.md) and [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) first. The short version: every subsystem that owns a resource is expected to be able to explain, on demand, why that resource still exists and what would break if it were removed. New code is reviewed against that bar, not just against "does it work".

## Ways to contribute

- **Bug reports.** Use the bug report issue template. A `selftest` failure, a panic screen, or a `whyalive`/`blast` output that looks wrong is exactly the kind of report this project wants, include it verbatim.
- **Fixing a known limitation.** The README keeps an honest "what's implemented" table. Anything marked `Experimental` or `Partial` is fair game. Three concrete ones found while writing these docs, real, not hypothetical:
  - `blast <pid>` reports shared file descriptors as `(unknown)`, it doesn't resolve them back to a file name yet, only the fd number and ref count (`kernel/Process/Blast/blast.c`).
  - `blast` also shows a `ref_count` display underflow (`4294967295`) on a safely-shared fd with a ref count of 0, a `uint32_t` wrapping below zero somewhere in the display path, not an actual reference count.
  - `quarantine` can't currently be triggered live from the shell: every built-in that opens a file (`cat`, `touch`, `write`) closes it immediately, and no bundled `User/Programs` test binary leaks fds on purpose. A small test program that opens several files in a loop without closing them would make the quarantine path demonstrable end to end.
- **New introspection tools.** If you're working on a subsystem and find yourself wishing you could ask it a question the way you can ask `whyalive` or `blast`, that instinct is usually correct. Propose it as an issue first, see the pattern in `kernel/Process/WhyAlive` before building your own.
- **Documentation.** If something in `docs/` doesn't match the actual code, that's a real bug, please open an issue or a PR even if you're not fixing the code itself.

## Development setup

See [`docs/BUILDING.md`](docs/BUILDING.md) for the full toolchain setup and build steps. In short:

```bash
git clone https://github.com/Mobeen0119/Aevros.git
cd Aevros
./build.sh
qemu-system-i386 -cdrom aevrosos.iso
```

## Coding conventions

- **C is freestanding.** No host libc, no exceptions, no floating point or SIMD register use (the context switch doesn't save that state, see `docs/ARCHITECTURE.md`). Look at the `gcc` flags in `build.sh` before assuming a standard library function is available; most aren't, and the freestanding replacements live in `Lib/`.
- **Match the surrounding file's style.** This codebase doesn't (yet) enforce a formatter. Keep brace style, naming, and indentation consistent with the file you're editing rather than importing a different convention.
- **Prefer explicit state over implicit assumptions.** This is the single most consistent pattern in the existing code: track *who* owns a resource and *when* something happened, don't just track *that* it happened. See `kernel/Process/TaskLife` and `kernel/Memory/KallocTracker` for the pattern to follow.
- **Every new subsystem directory gets a matching shell command.** If you add `kernel/Process/Something/`, add a dispatch entry and a `help_line(...)` in `kernel/Shell/shell.c` in the same PR. A subsystem nobody can reach from the shell is much harder for the next contributor to learn from.
- **New features ship with a self-test.** Add a `test_<feature>` function to `kernel/selftest.c` following the existing `CHECK(name, condition)` pattern, and wire it into `selftest_run()`. If a feature genuinely can't be exercised from a self-test (for example, something that needs a live interrupt frame, see `test_fork`'s `[SKIP]` for a real example), say so explicitly with a comment rather than silently omitting a test.
- **No `--` in prose.** In comments, commit messages, and documentation, write full words or use a comma or parenthesis instead of a double hyphen. This is a house style choice for readability, not a hard technical constraint, but PRs should follow it in anything meant to be read.

## Commit and PR process

1. **Open an issue first** for anything beyond a small fix (typo, obvious bug, doc correction). For new subsystems or anything touching boot order, memory layout, or the syscall ABI, a short design discussion up front saves a much longer review later.
2. **Keep PRs scoped.** One subsystem or one fix per PR. A PR that adds a feature and reformats unrelated files is harder to review and harder to revert if something's wrong.
3. **Write commit messages that explain *why*, not just *what*.** "Fix page fault handler" tells a reviewer nothing; "Fix page fault handler treating a write to a null pointer as a protection fault instead of an unmapped access" does.
4. **Run `selftest` before opening the PR**, and paste its output (or a summary) in the PR description if you touched anything it covers.
5. **Don't commit build artifacts.** `.gitignore` excludes `*.o`, `*.iso`, `*.log`, `kernel.elf`, `iso/`, and `build_tmp/`. If your `git status` shows any of those as new files, something's off with your build, not the ignore rules.
6. **Update the docs in the same PR**, not a follow-up. If you add a shell command, add its story to `docs/COMMANDS.md`. If you change boot order or a subsystem's responsibilities, update the relevant diagram in `docs/ARCHITECTURE.md`. A PR that changes behavior without updating the doc that describes that behavior will be asked to add it before merge.

## Reporting a security issue

Don't open a public issue for a security-relevant bug (something exploitable from user-mode code, for example). See [`SECURITY.md`](SECURITY.md).

## Code of conduct

Participation in this project, issues, PRs, and any other project space, is governed by [`CODE_OF_CONDUCT.md`](CODE_OF_CONDUCT.md).

## Questions

If you're not sure where something belongs, or whether an idea fits the project, open a draft issue describing what you want to do before writing the implementation. That's cheaper for everyone than a large PR that turns out to need a different design.