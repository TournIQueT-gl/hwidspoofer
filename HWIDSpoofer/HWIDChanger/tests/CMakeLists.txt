cmake_minimum_required(VERSION 3.16)
project(HWIDReadbackParserTests LANGUAGES C)

# Portable test target: no Windows APIs, registry changes, drivers or hooks.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(hwid_readback_tests
    test_readback.c
    ../hwid_readback.c
)
if(MSVC)
    target_compile_options(hwid_readback_tests PRIVATE /W4)
else()
    target_compile_options(hwid_readback_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME readback_parser COMMAND hwid_readback_tests)

# Run malformed-buffer stress tests separately from the basic assertions.
add_executable(hwid_readback_stress
    fuzz_readback.c
    ../hwid_readback.c
)
if(MSVC)
    target_compile_options(hwid_readback_stress PRIVATE /W4)
else()
    target_compile_options(hwid_readback_stress PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME readback_stress COMMAND hwid_readback_stress)

# Pure preview validation: does not boot a VM or modify any hardware.
find_program(WINDOWS_POWERSHELL NAMES powershell pwsh)
if(WINDOWS_POWERSHELL)
    add_test(NAME qemu_profile_preview
        COMMAND "${WINDOWS_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/Test-VirtualHardwareProfile.ps1")
endif()
