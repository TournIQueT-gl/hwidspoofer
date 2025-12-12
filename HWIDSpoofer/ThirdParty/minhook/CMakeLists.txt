# MinHook库
# 从 https://github.com/TsudaKageworthy/minhook 获取源码
# 或使用vcpkg安装: vcpkg install minhook

cmake_minimum_required(VERSION 3.16)
project(minhook C)

# 如果源文件存在则编译，否则尝试使用系统安装的库
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/hook.c")
    message(STATUS "Building MinHook from source")
    
    set(MINHOOK_SOURCES
        src/hook.c
        src/buffer.c
        src/trampoline.c
        src/hde/hde32.c
        src/hde/hde64.c
    )
    
    add_library(minhook STATIC ${MINHOOK_SOURCES})
    target_include_directories(minhook PUBLIC include)
    
else()
    message(STATUS "MinHook source not found, creating interface library")
    message(STATUS "Please download MinHook from: https://github.com/TsudaKageworthy/minhook")
    message(STATUS "Or install via vcpkg: vcpkg install minhook")
    
    # 创建接口库，假设MinHook已通过其他方式安装
    add_library(minhook INTERFACE)
    
    # 尝试查找系统安装的MinHook
    find_path(MINHOOK_INCLUDE_DIR MinHook.h)
    find_library(MINHOOK_LIBRARY NAMES minhook MinHook)
    
    if(MINHOOK_INCLUDE_DIR AND MINHOOK_LIBRARY)
        target_include_directories(minhook INTERFACE ${MINHOOK_INCLUDE_DIR})
        target_link_libraries(minhook INTERFACE ${MINHOOK_LIBRARY})
    else()
        message(WARNING "MinHook not found. Please install it manually.")
    endif()
endif()

