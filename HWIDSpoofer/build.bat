@echo off
setlocal enabledelayedexpansion

echo ============================================
echo    HWID Spoofer Build Script
echo ============================================
echo.

:: 检查CMake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found. Please install CMake first.
    echo Download: https://cmake.org/download/
    pause
    exit /b 1
)

:: 检查Visual Studio
set VS_FOUND=0
if exist "C:\Program Files\Microsoft Visual Studio\2022" set VS_FOUND=1
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019" set VS_FOUND=1
if exist "C:\Program Files\Microsoft Visual Studio\2019" set VS_FOUND=1

if %VS_FOUND%==0 (
    echo [WARNING] Visual Studio not found in default location.
    echo Make sure you have Visual Studio 2019 or later installed.
)

:: 创建构建目录
echo [1/4] Creating build directories...
if not exist "build" mkdir build
if not exist "build\x64" mkdir build\x64
if not exist "build\x86" mkdir build\x86

:: 构建64位版本
echo.
echo [2/4] Building x64 version...
cd build\x64
cmake ../.. -A x64
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed for x64
    cd ..\..
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed for x64
    cd ..\..
    pause
    exit /b 1
)
cd ..\..

:: 构建32位版本
echo.
echo [3/4] Building x86 version...
cd build\x86
cmake ../.. -A Win32
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed for x86
    cd ..\..
    pause
    exit /b 1
)

cmake --build . --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed for x86
    cd ..\..
    pause
    exit /b 1
)
cd ..\..

:: Configuration is already copied by CMake configure_file into bin/.
:: Avoid copying to a non-existent Release subdirectory.
:: 复制配置文件
echo.
echo [4/4] Copying config files...
if not exist "build\x64\bin\hwid_config.json" (
    echo [ERROR] Missing x64 configuration output
    exit /b 1
)
if not exist "build\x86\bin\hwid_config.json" (
    echo [ERROR] Missing x86 configuration output
    exit /b 1
)

echo.
echo ============================================
echo    Build Complete!
echo ============================================
echo.
echo Output files:
echo   x64: build\x64\bin\Release\
echo   x86: build\x86\bin\Release\
echo.
echo Files:
echo   - HWIDInjector.exe  (Injector tool)
echo   - SpooferDLL.dll    (Hook DLL)
echo   - hwid_config.json  (Configuration)
echo.
echo Usage:
echo   HWIDInjector.exe -n target.exe
echo.

pause

