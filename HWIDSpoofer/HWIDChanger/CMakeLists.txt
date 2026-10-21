cmake_minimum_required(VERSION 3.16)
project(HWIDChanger VERSION 2.0.0 LANGUAGES C)
if(NOT WIN32)
    message(FATAL_ERROR "HWIDChanger v2 requires Windows.")
endif()
# The current hook implementation uses GCC statement expressions.
if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
    message(FATAL_ERROR "HWIDChanger v2 requires MinGW-w64 GCC (not MSVC).")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CHANGER_COMMON_SOURCES
    hwid_core.c hwid_hooks.c hwid_readback.c registry_spoof.c appinit_spoof.c)
add_executable(HWIDChanger main.c ${CHANGER_COMMON_SOURCES})
target_compile_definitions(HWIDChanger PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(HWIDChanger PRIVATE -Wall -Wextra)
target_link_libraries(HWIDChanger PRIVATE kernel32 advapi32 iphlpapi user32)
add_library(HWIDChangerHook SHARED hwid_dll.c)
set_target_properties(HWIDChangerHook PROPERTIES OUTPUT_NAME "HWIDChanger" PREFIX "")
target_link_libraries(HWIDChangerHook PRIVATE kernel32 advapi32 iphlpapi)
add_executable(HWIDTest hwid_test.c hwid_readback.c)
target_link_libraries(HWIDTest PRIVATE kernel32 advapi32 iphlpapi)

# Build and register the standalone read-only parser tests.
enable_testing()
add_subdirectory(tests)
install(TARGETS HWIDChanger HWIDChangerHook HWIDTest
    RUNTIME DESTINATION bin LIBRARY DESTINATION bin)

# Both PowerShell helpers are Windows inbox dependencies for the console UI.
foreach(HELPER Verify-HWID.ps1 MacOverride.ps1 VirtualHardwareProfile.ps1 Verify-VirtualHardwareGuest.ps1)
    add_custom_command(TARGET HWIDChanger POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/${HELPER}"
        "$<TARGET_FILE_DIR:HWIDChanger>/${HELPER}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/tools/${HELPER}" DESTINATION bin)
endforeach()
