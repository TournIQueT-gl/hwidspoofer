@echo off
setlocal
cd /d "%~dp0"

echo ============================================
echo    HWID Spoofer Legacy Build (MinHook)
echo ============================================
echo This produces SpooferDLL / HWIDInjector, not HWIDChanger v2.
echo For v2 use HWIDChanger\build.bat
echo.

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found in PATH.
    exit /b 1
)
for %%A in (x64 x86) do (
    if not exist "build\%%A" mkdir "build\%%A"
    cmake -S . -B "build\%%A" -A %%A
    if errorlevel 1 (
        echo [ERROR] Configure failed for %%A. Verify Visual Studio and MinHook are installed.
        exit /b 1
    )
    cmake --build "build\%%A" --config Release
    if errorlevel 1 (
        echo [ERROR] Build failed for %%A.
        exit /b 1
    )
    if not exist "build\%%A\bin\Release" (
        echo [ERROR] No Release output directory for %%A.
        exit /b 1
    )
    if not exist "build\%%A\bin\hwid_config.json" (
        echo [ERROR] Missing generated JSON config for %%A.
        exit /b 1
    )
    copy /Y "build\%%A\bin\hwid_config.json" "build\%%A\bin\Release\hwid_config.json" >nul
    if errorlevel 1 (
        echo [ERROR] Could not copy config for %%A.
        exit /b 1
    )
)
echo [OK] Legacy x64 and x86 builds completed.
exit /b 0
