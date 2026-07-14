# Security Policy

## Security Overview

Aevros is an experimental operating system kernel intended for research, education, and systems programming. It is under active development and should not be considered production-ready.

Do not use Aevros to process sensitive data or deploy it in security-critical environments. While many kernel components are implemented, the project is still evolving and security hardening is ongoing.

Security vulnerabilities affecting core kernel functionality should be reported responsibly rather than through the public issue tracker.

---

## Reporting a Vulnerability

If you discover a potential security vulnerability, **do not create a public GitHub issue**.

Instead, report it privately by one of the following methods:

1. Open a **GitHub Security Advisory** for this repository (preferred), if available.
2. Contact the project maintainer directly through GitHub: **@Mobeen0119**.

Please include:

- Affected subsystem (for example: Paging, VFS, ELF Loader, System Calls, Memory Manager)
- Steps required to reproduce the issue
- Expected behavior
- Actual behavior
- Logs, screenshots, or test binaries if applicable
- Any proposed fix or analysis

---

## Scope

This policy applies to the Aevros source tree, including:

- boot/
- kernel/
- Include/
- Lib/
- User/
- build.sh
- MakeFile
- linker.ld

Issues involving external software such as QEMU, GRUB, GCC, Clang, or other third-party tools should be reported to their respective maintainers.

---

## Disclosure

Please allow reasonable time for the vulnerability to be investigated and, when appropriate, fixed before publicly disclosing technical details.

Responsible disclosure helps protect users and contributors while improvements are being developed.

---

## Security Goals

Aevros aims to continually improve:

- Memory safety
- Process isolation
- Privilege separation
- System call validation
- Resource ownership tracking
- Kernel diagnostics and observability

As the project evolves, additional security mechanisms and hardening features will be introduced.