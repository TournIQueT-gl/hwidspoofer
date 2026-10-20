# Security Policy

## Supported branch

Security and reliability fixes are developed on `safety-hardening` until they have been reviewed. The `master` branch is not changed by this policy.

## Reporting a vulnerability

Please use GitHub's private vulnerability reporting feature for this repository if it is enabled. Otherwise, contact the repository maintainer privately through a channel listed on their GitHub profile. Do not publish exploit details or personal/system identifiers in a public issue.

Include:
- affected file or component and commit/ref;
- Windows version, architecture, compiler, and build configuration;
- minimal reproduction steps using a disposable VM or synthetic test data;
- observed behavior and expected behavior;
- whether the issue can be reproduced from a clean checkout.

Do not attach real hardware identifiers, credentials, private keys, or unredacted system logs.

## Safe testing

- Do not run untrusted checked-in executables or drivers on a primary machine.
- Use a disposable VM with snapshots for Windows integration testing.
- Do not enable global registry-based DLL loading or load an unsigned/experimental kernel driver on a daily-use machine.
- Prefer synthetic fixtures and tests that do not alter hardware identity or system configuration.

## Scope

Reports about memory safety, privilege boundaries, crash risks, unsafe cleanup, build integrity, and unintended system-wide side effects are in scope. This project is not intended to defeat third-party game, anti-cheat, licensing, or security enforcement systems.
