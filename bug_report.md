---
name: Bug report
about: Something in Aevros doesn't behave the way it should
title: "[bug] "
labels: bug
assignees: ''
---

## What happened

A clear description of the problem. If the kernel produced a panic screen, a `selftest` failure, or output from a diagnostic command like `whyalive`, `blast`, or `memstory`, paste it verbatim below rather than summarizing it, the exact wording usually matters.

```text
(paste panic output, selftest output, or command output here)
```

## What you expected instead

## Steps to reproduce

1.
2.
3.

Include the exact shell commands run inside Aevros, in order. If it depends on a specific test binary or file, mention which one.

## Environment

- How you built it: `./build.sh` output was clean? Any warnings?
- How you ran it: `qemu-system-i386 -cdrom aevrosos.iso` or `-kernel kernel.elf`, or real hardware?
- Host OS and QEMU version (`qemu-system-i386 --version`), if relevant:
- Commit or branch you're on (`git rev-parse HEAD`):

## Anything else

Logs, screenshots of the VGA output, or a link to a branch that reproduces it, if you have one.
