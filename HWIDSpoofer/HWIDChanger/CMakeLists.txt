cmake_minimum_required(VERSION 3.16)
project(HWIDChanger C)

# 使用静态运行时库（减少依赖）
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Release optimization. Keep the MSVC stack security cookie enabled (/GS).
# Do not disable compiler security checks to chase marginal performance gains.
if(MSVC)
    string(APPEND CMAKE_C_FLAGS_RELEASE " /O2 /GL /GS /Gy")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /LTCG /OPT:REF /OPT:ICF")
endif()
# 定义
add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)

# 源文件
set(SOURCES
    main.c
    hwid_core.c
    hwid_hooks.c
)

# 主可执行文件
add_executable(HWIDChanger ${SOURCES})

target_link_libraries(HWIDChanger PRIVATE
    kernel32
    advapi32
    iphlpapi
)

# 设置为控制台程序
set_target_properties(HWIDChanger PROPERTIES
    WIN32_EXECUTABLE FALSE
    OUTPUT_NAME "HWIDChanger"
)

# 安装
install(TARGETS HWIDChanger RUNTIME DESTINATION bin)

# 驱动项目（需要WDK，单独构建）
message(STATUS "
================================================================================
  HWIDChanger - 构建说明
================================================================================
  
  用户态程序:
    cmake --build . --config Release
  
  内核驱动 (需要WDK):
    cd driver
    使用Visual Studio + WDK构建
  
  或使用提供的build.bat一键构建
================================================================================
")

