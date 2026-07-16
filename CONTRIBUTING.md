# Contributing to Aevros

So... you want to mess with an operating system.

It's a small(now) kernel built to answer a simple question:

> **Why did that happen?**

Most kernels stop after showing you *what* happened. Aevros tries to explain *why*, even when the answer is "because you just broke something."

If that idea sounds fun, you're in the right place.

---

## Where can I help?

Almost anywhere.

1- Found a bug? Open an issue.

2- See something marked **Experimental** or **Partial** in the README? That's basically an invitation.

3- Ever wished the kernel could answer another question about itself? Chances are that's a feature worth discussing.

4- Spotted documentation that doesn't match the code? That's a bug too.

---

## Before writing code

Take a quick look at:

- `docs/PHILOSOPHY.md`
- `docs/ARCHITECTURE.md`

They're much shorter than debugging code you misunderstood.

---

## A few ground rules

Keep things simple.

- Match the style of nearby code.
- Keep one feature or fix per PR.
- Add documentation for new features.
- Add a self-test whenever it makes sense.
- Explain **why** in your commit messages.
- Please don't commit build files. Git has enough trust issues already.

---

## Big ideas?

Awesome.

Open an issue before writing hundreds of lines of code. Ten minutes of discussion can save ten hours of rewriting.

---

## Security

If you discover a security issue, **please don't post it publicly.**

See `SECURITY.md` for how to report it responsibly.

---

## One last thing

This project exists because operating systems become a lot less mysterious once you can ask them questions.

If your contribution helps someone understand the system a little better, you've already made Aevros better.