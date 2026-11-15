# Contributing to KelpOS

Thanks for wanting to contribute! KelpOS is GPL-3.0 — all contributions
end up under the same license.

## Process

1. Fork / branch (`feature/<name>` or `fix/<name>`).
2. Small, focused commits with concise messages.
3. `make test` and `make test-qemu` must stay green.
4. Open a pull request with a short description.

## Style

- C17, `snake_case`, no tab characters in source code (Makefile: tabs!).
- Respect freestanding rules: don't use C standard library functions
  outside `klibc/`.
- Keep kernel APIs small — build abstractions only once a second
  consumer exists.
- Every new feature should be testable (host tests preferred,
  QEMU smoke test for booting).

## Commit format

`<area>: short description` — e.g. `kernel: idt errno fix`,
`klibc: strncmp corner case`, `build: qemu headless target`.

## Code of Conduct

By participating you agree to our [Code of Conduct](CODE_OF_CONDUCT.md).
AI-assisted contributions are welcome under the rules in
[AI-POLICY.md](AI-POLICY.md).
