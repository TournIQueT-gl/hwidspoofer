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
