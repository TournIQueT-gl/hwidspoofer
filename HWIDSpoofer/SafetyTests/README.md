# Isolated safety tests

These tests exercise a small bounded parser against **synthetic byte arrays only**. They do not read hardware identifiers, alter system state, inject into processes, or connect to a game or anti-cheat. The helper is deliberately not wired into the existing HWID-changing code; it is a starting point for reviewing parser contracts, not proof that the production parser is fixed.

## Run

From this directory:

- MSVC Developer Command Prompt: `cl /std:c++17 /EHsc /W4 test_smbios_record_parser.cpp`
- GCC or Clang: `c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror test_smbios_record_parser.cpp -o parser_tests`

Then run `parser_tests.exe` on Windows or `./parser_tests` on Linux/macOS.

## Covered cases

- Null and truncated input
- Invalid formatted lengths
- Missing string-set terminator
- A well-formed synthetic record
- End-of-table record recognition

The production SMBIOS parsing path remains unchanged and still requires a separate, carefully reviewed refactor. These tests are not a substitute for ASan/UBSan, fuzzing, or Windows VM testing.
