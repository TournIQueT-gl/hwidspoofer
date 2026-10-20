cmake_minimum_required(VERSION 3.16)
project(minhook_wrapper LANGUAGES C)
# The upstream source is not currently committed to this repository.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/hook.c")
    add_library(minhook STATIC
        src/hook.c src/buffer.c src/trampoline.c
        src/hde/hde32.c src/hde/hde64.c)
    target_include_directories(minhook PUBLIC include)
else()
    find_path(MINHOOK_INCLUDE_DIR NAMES MinHook.h)
    find_library(MINHOOK_LIBRARY NAMES minhook MinHook)
    if(NOT MINHOOK_INCLUDE_DIR OR NOT MINHOOK_LIBRARY)
        message(FATAL_ERROR
            "MinHook sources are absent and no installed MinHook library found. "
            "Use vcpkg with CMAKE_TOOLCHAIN_FILE, or obtain official source "
            "from https://github.com/TsudaKageyu/minhook.")
    endif()
    add_library(minhook INTERFACE)
    target_include_directories(minhook INTERFACE "${MINHOOK_INCLUDE_DIR}")
    target_link_libraries(minhook INTERFACE "${MINHOOK_LIBRARY}")
endif()
