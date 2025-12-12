# 内核驱动需要使用WDK编译
# 此CMakeLists.txt仅作参考，实际编译请使用Visual Studio + WDK

# 或者使用以下命令（需要安装WDK）:
# 
# 1. 打开"Developer Command Prompt for VS"
# 2. cd到SpooferDriver目录
# 3. 运行: msbuild HWIDSpooferDriver.vcxproj /p:Configuration=Release /p:Platform=x64

cmake_minimum_required(VERSION 3.16)

message(WARNING "
======================================================================
内核驱动需要使用Windows Driver Kit (WDK)编译。
请使用Visual Studio + WDK，或者使用提供的.inf和Sources文件。

安装WDK: https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
======================================================================
")

# 这里只是占位，实际不会构建
project(HWIDSpooferDriver C)

# 源文件列表（仅供参考）
set(DRIVER_SOURCES
    driver.c
    disk_spoof.c
    smbios_spoof.c
)

message(STATUS "Driver source files: ${DRIVER_SOURCES}")

