cmake_minimum_required(VERSION 3.16)
project(HWIDChanger VERSION 2.0.0 LANGUAGES C)
if(NOT WIN32)
    message(FATAL_ERROR "HWIDChanger v2 requires Windows.")
endif()
# Build the supported Windows diagnostics and reversible registry/MAC features.
# The unvalidated remote injection and inline-patching engine is not linked.
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "This release currently requires MinGW-w64 GCC.")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CHANGER_COMMON_SOURCES
    hwid_core.c hwid_readers_win.c hwid_backend_unavailable.c
    hwid_readback.c registry_spoof.c appinit_spoof.c)
add_executable(HWIDChanger main.c ${CHANGER_COMMON_SOURCES})
target_compile_definitions(HWIDChanger PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(HWIDChanger PRIVATE -Wall -Wextra)
target_link_libraries(HWIDChanger PRIVATE kernel32 advapi32 iphlpapi user32)
add_executable(HWIDTest hwid_test.c hwid_readback.c)
target_link_libraries(HWIDTest PRIVATE kernel32 advapi32 iphlpapi)

# Build and register the standalone read-only parser tests.
enable_testing()
add_subdirectory(tests)
install(TARGETS HWIDChanger HWIDTest
    RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

# Both PowerShell helpers are Windows inbox dependencies for the console UI.
foreach(HELPER Verify-HWID.ps1 MacOverride.ps1 VirtualHardwareProfile.ps1 Verify-VirtualHardwareGuest.ps1)
    add_custom_command(TARGET HWIDChanger POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/${HELPER}"
        "$<TARGET_FILE_DIR:HWIDChanger>/${HELPER}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tools/${HELPER}" DESTINATION bin)
endforeach()
