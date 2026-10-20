@echo off
setlocal
cd /d "%~dp0"
set "BIN=bin"
where gcc >nul 2>&1
if errorlevel 1 (
    echo [ERROR] MinGW-w64 GCC missing from PATH.
    exit /b 1
)
if not exist "%BIN%" mkdir "%BIN%"

echo [1/3] Building HWIDChanger.exe...
gcc -O2 -s -static -ffunction-sections -fdata-sections "-Wl,--gc-sections" ^
    -o "%BIN%\HWIDChanger.exe" ^
    main.c hwid_core.c hwid_hooks.c registry_spoof.c appinit_spoof.c ^
    -liphlpapi -ladvapi32 -luser32
if errorlevel 1 exit /b 1

echo [2/3] Building HWIDChanger.dll...
gcc -O2 -s -shared -static -ffunction-sections -fdata-sections "-Wl,--gc-sections" ^
    -o "%BIN%\HWIDChanger.dll" hwid_dll.c -liphlpapi -ladvapi32
if errorlevel 1 exit /b 1

echo [3/3] Building read-only HWIDTest.exe...
gcc -O2 -s -static -o "%BIN%\HWIDTest.exe" hwid_test.c -liphlpapi -ladvapi32
if errorlevel 1 exit /b 1

for %%F in (HWIDChanger.exe HWIDChanger.dll HWIDTest.exe) do (
    if not exist "%BIN%\%%F" (
        echo [ERROR] Missing required release artifact: %%F
        exit /b 1
    )
)
echo [OK] v2.0 build completed.
exit /b 0
