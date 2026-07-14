## What this changes

A clear description of the change and why it's needed.

## Related issue

Closes #

## Checklist

- [ ] I've read `CONTRIBUTING.md`
- [ ] The change builds cleanly with `./build.sh` (no new warnings)
- [ ] I ran `selftest` in the booted kernel and it passes
- [ ] If I added a subsystem, it has a matching shell command in `kernel/Shell/shell.c` (including a `help_line(...)` entry)
- [ ] If I added a feature, it has a matching `test_<feature>` in `kernel/selftest.c`, or a comment explaining why it can't be self-tested
- [ ] If this changes behavior described in `docs/`, I updated the relevant doc (`docs/ARCHITECTURE.md`, `docs/COMMANDS.md`, `docs/BUILDING.md`) in this same PR
- [ ] No build artifacts (`*.o`, `*.iso`, `*.log`, `kernel.elf`, `iso/`, `build_tmp/`) are included in the diff

## How you tested it

Commands run inside Aevros, or the `selftest` output, showing the change works.

```text
(paste relevant output here)
```

## Anything reviewers should look at closely
