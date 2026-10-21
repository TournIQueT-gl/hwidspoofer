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

rem This supported build does not publish the unvalidated legacy injection DLL.
if exist "%BIN%\HWIDChanger.dll" (
    echo [CLEAN] Removing obsolete legacy injection DLL from output folder...
    del /f /q "%BIN%\HWIDChanger.dll"
    if exist "%BIN%\HWIDChanger.dll" exit /b 1
)

echo [1/3] Building HWIDChanger.exe with read-only native backend...
gcc -O2 -s -static -ffunction-sections -fdata-sections "-Wl,--gc-sections" ^
    -o "%BIN%\HWIDChanger.exe" ^
    main.c hwid_core.c hwid_readers_win.c hwid_backend_unavailable.c ^
    hwid_readback.c registry_spoof.c appinit_spoof.c ^
    -liphlpapi -ladvapi32 -luser32
if errorlevel 1 exit /b 1

echo [2/3] Building read-only HWIDTest.exe...
gcc -O2 -s -static -o "%BIN%\HWIDTest.exe" hwid_test.c hwid_readback.c -liphlpapi -ladvapi32
if errorlevel 1 exit /b 1

for %%F in (HWIDChanger.exe HWIDTest.exe) do (
    if not exist "%BIN%\%%F" (
        echo [ERROR] Missing required release artifact: %%F
        exit /b 1
    )
)
echo [3/3] Bundling verification, recovery and isolated-guest tools...
for %%F in (Verify-HWID.ps1 MacOverride.ps1 VirtualHardwareProfile.ps1 Verify-VirtualHardwareGuest.ps1) do (
    if not exist "tools\%%F" (
        echo [ERROR] Missing companion script: %%F
        exit /b 1
    )
    copy /Y "tools\%%F" "%BIN%\%%F" >nul
    if errorlevel 1 (
        echo [ERROR] Could not copy %%F to bin.
        exit /b 1
    )
)
echo [TEST] Checking SMBIOS and storage descriptor parsing...
gcc -std=c11 -Wall -Wextra -o "%BIN%\HWIDReadbackTests.exe" hwid_readback.c tests\test_readback.c
if errorlevel 1 exit /b 1
"%BIN%\HWIDReadbackTests.exe"
if errorlevel 1 exit /b 1

echo [TEST] Guest hardware profile command-line preview...
powershell -NoProfile -ExecutionPolicy Bypass -File "tests\Test-VirtualHardwareProfile.ps1"
if errorlevel 1 exit /b 1

echo [OK] Supported v2.0 build completed; unsafe native interception remains unavailable.
exit /b 0
