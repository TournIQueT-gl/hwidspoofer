@echo off
REM ============================================================
REM  HWID Spoofer 内核驱动编译脚本
REM  需要安装 Windows Driver Kit (WDK)
REM ============================================================

echo.
echo ============================================
echo   HWID Spoofer Driver Build Script
echo ============================================
echo.

REM 检查 WDK 是否安装
set WDK_PATH=C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64
if not exist "%WDK_PATH%" (
    set WDK_PATH=C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64
)
if not exist "%WDK_PATH%" (
    echo [ERROR] Windows Driver Kit (WDK) not found!
    echo.
    echo Please install WDK from:
    echo https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
    echo.
    pause
    exit /b 1
)

echo [OK] Found WDK at: %WDK_PATH%
echo.

REM 设置编译环境
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)
if errorlevel 1 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

echo [1/3] Compiling driver sources...

REM 创建输出目录
if not exist "bin" mkdir bin

REM 编译驱动 (使用 WDK 的 cl.exe)
cl /c /kernel /GS- /W4 /WX- /Zi /Od ^
   /D "NTDDI_VERSION=0x0A000000" /D "_WIN64" /D "_AMD64_" /D "AMD64" ^
   /D "DEPRECATE_DDK_FUNCTIONS=1" /D "MSC_NOOPT" /D "_WIN32_WINNT=0x0A00" ^
   /D "WINVER=0x0A00" /D "WINNT=1" /D "NTDDI_WIN10_RS5=0x0A000006" ^
   /I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\km" ^
   /Fo"bin\\" ^
   driver.c irp_hook.c disk_spoof.c smbios_spoof.c

if errorlevel 1 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo [2/3] Linking driver...

link /DRIVER /SUBSYSTEM:NATIVE /ENTRY:DriverEntry ^
     /OUT:"bin\HWIDSpoofer.sys" ^
     /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\km\x64" ^
     bin\driver.obj bin\irp_hook.obj bin\disk_spoof.obj bin\smbios_spoof.obj ^
     ntoskrnl.lib hal.lib wmilib.lib

if errorlevel 1 (
    echo [ERROR] Linking failed!
    pause
    exit /b 1
)

echo [3/3] Driver built successfully!
echo.
echo Output: bin\HWIDSpoofer.sys
echo.
echo ============================================
echo  Next Steps:
echo ============================================
echo  1. Enable test signing:
echo     bcdedit /set testsigning on
echo     (Reboot required)
echo.
echo  2. Copy driver to HWIDChanger folder:
echo     copy bin\HWIDSpoofer.sys ..\HWIDChanger\bin\
echo.
echo  3. Run HWIDChanger with global mode:
echo     HWIDChanger.exe -r --enable-global
echo ============================================
echo.

pause

