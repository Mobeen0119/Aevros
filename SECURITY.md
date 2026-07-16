# Security Policy

## Is Aevros secure?

 Not yet.

Aevros is an experimental hobby kernel. It doesn't have full user/kernel isolation yet, so **don't run it on real hardware with data you care about** and don't expose it to a network. It has no network stack today, but that's the rule going forward too.

If you find something serious—memory corruption, privilege escalation, kernel memory leaks through syscalls, ELF loader bugs, or anything that could become a real security issue—please report it privately instead of opening a public issue.

## Reporting a vulnerability

Please **don't** post security bugs publicly.

Instead:

1. Open a GitHub Security Advisory (preferred), if available.
2. Otherwise, contact **@Mobeen0119** directly on GitHub.

Include:
- What broke
- How to reproduce it
- Expected vs actual behavior
- A fix, if you have one

## Scope

This policy covers the Aevros kernel and its build scripts. Bugs in QEMU, GRUB, or your compiler belong to those projects, not Aevros.