---
name: Feature request
about: Propose a new subsystem, shell command, or capability
title: "[feature] "
labels: enhancement
assignees: ''
---

## What are you proposing

A clear description of the capability you want to add.

## What question does it answer

Aevros's introspection tools (`whyalive`, `blast`, `quarantine`, and similar, see `docs/PHILOSOPHY.md`) are each built around one specific question a contributor might ask about a running resource. If this is a new tool in that spirit, what's the one-sentence question it answers?

## Where it fits

Which existing subsystem does this extend or sit next to (`kernel/Process`, `kernel/Memory`, `kernel/VFS`, `kernel/Shell`, a new driver, something else)? See `docs/ARCHITECTURE.md` if you're not sure.

## Proposed shell command (if applicable)

```text
Aevros > your-command <args>
(example of what it would print)
```

## Alternatives considered

Is there an existing command that almost does this? Why doesn't it cover the case you have in mind?

## Are you willing to work on this

Yes / No / Yes, with some guidance on where to start
