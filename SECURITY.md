# Security Policy

## About Aevros's security model

Aevros is an experimental, educational kernel. It does not yet have full ring 3 isolation (see the "what's implemented" table in the [README](README.md)), so treat it the way you'd treat any hobby kernel: **do not run it on real hardware with data or credentials you care about, and do not expose it to any network.** It has no network stack today, but this guidance is written to hold as one gets added.

That said, bugs that would matter even in a hardened version, memory corruption reachable from user code, a syscall that lets user-mode code read or write kernel memory it shouldn't, an integer overflow in the ELF loader that could be used to smuggle in unexpected code, are worth reporting responsibly rather than as a public issue, so a fix can go out before the details are public.

## Reporting a vulnerability

**Please do not open a public GitHub issue for a security-relevant bug.**

Instead:

1. Open a [GitHub Security Advisory](https://github.com/Mobeen0119/Aevros/security/advisories/new) for this repository if the option is available to you, this is the preferred path and keeps the report private until a fix is ready.
2. If that's not available, contact the maintainer directly through their GitHub profile (`@Mobeen0119`) rather than the public issue tracker.

When reporting, please include:

- Which subsystem is affected (e.g. "ELF loader", "syscall dispatch", "paging")
- Steps to reproduce, ideally including the exact shell commands or the test binary used
- What you expected to happen versus what actually happened
- Whether you have a proposed fix

## What to expect

This is a small, actively developed hobby project without a dedicated security team, so please be patient. You should get an acknowledgment of your report within a reasonable time, and the maintainer will work with you on a fix and a coordinated disclosure timeline before any public write-up.

## Scope

This policy covers the Aevros kernel itself (everything under `boot/`, `kernel/`, `Drivers/`, `Lib/`, `Include/`, `User/`) and its build tooling (`build.sh`, `MakeFile`). It does not cover third-party tools you use to build or run it (your compiler, QEMU, GRUB), report those upstream instead.