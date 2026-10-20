# Isolated safety tests

These tests exercise a small bounded parser against **synthetic byte arrays only**. They do not read hardware identifiers, alter system state, inject into processes, or connect to a game or anti-cheat. The helper is deliberately not wired into the existing HWID-changing code; it is a starting point for reviewing parser contracts, not proof that the production parser is fixed.

## Build and run with CMake

From a terminal:

```sh
cmake -S HWIDSpoofer/SafetyTests -B build/safety-tests
cmake --build build/safety-tests --config Release
ctest --test-dir build/safety-tests -C Release --output-on-failure
```

The test target uses C++17 and treats compiler warnings as errors. On GCC/Clang builds, CI also enables AddressSanitizer and UndefinedBehaviorSanitizer.

## Direct compiler commands

- MSVC Developer Command Prompt: `cl /nologo /std:c++17 /EHsc /W4 /WX test_smbios_record_parser.cpp /Fe:parser_tests.exe`
- GCC or Clang: `c++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-omit-frame-pointer test_smbios_record_parser.cpp -o parser_tests`

Then run `parser_tests.exe` on Windows or `./parser_tests` on Linux/macOS.

## Covered cases

- Null and truncated input
- Invalid formatted lengths
- Missing string-set terminator
- A well-formed synthetic record
- End-of-table record recognition
- A deterministic sweep of short malformed inputs and output-bound invariants
- 100,000 reproducible pseudo-random synthetic inputs
- Verification that failed parses clear every output field

The production SMBIOS parsing path remains unchanged and still requires a separate, carefully reviewed refactor. These tests are not a substitute for broader fuzzing or Windows VM testing.
