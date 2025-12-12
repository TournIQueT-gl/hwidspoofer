# HWID Changer Kernel Driver Makefile
# 使用WDK命令行工具编译

TARGETNAME = HWIDChanger
TARGETTYPE = DRIVER

SOURCES = hwid_driver.c

INCLUDES = $(DDK_INC_PATH)

TARGETLIBS = $(DDK_LIB_PATH)\ntoskrnl.lib

C_DEFINES = -DUNICODE -D_UNICODE

# 使用方法:
# 1. 打开 "x64 Free Build Environment" (WDK命令提示符)
# 2. cd 到此目录
# 3. 运行 build -cZg

