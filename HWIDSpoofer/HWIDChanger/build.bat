@echo off
setlocal

echo ======================================
echo  HWID Changer Build Script
echo ======================================

set GCC=C:\mingw64\bin\gcc.exe
set BIN=bin

if not exist "%GCC%" (
    echo [ERROR] GCC not found: %GCC%
    exit /b 1
)

if not exist "%BIN%" mkdir "%BIN%"

echo.
echo [1/2] Building HWIDChanger.exe ...
"%GCC%" -Os -s -static -ffunction-sections -fdata-sections "-Wl,--gc-sections" ^
    -o "%BIN%\HWIDChanger.exe" ^
    main.c hwid_core.c hwid_hooks.c registry_spoof.c ^
    -liphlpapi -ladvapi32

if errorlevel 1 (
    echo [ERROR] EXE compilation failed
    exit /b 1
)
echo [OK] HWIDChanger.exe

echo.
echo [2/2] Building HWIDChanger.dll ...
"%GCC%" -Os -s -shared -static -ffunction-sections -fdata-sections "-Wl,--gc-sections" ^
    -o "%BIN%\HWIDChanger.dll" ^
    hwid_dll.c ^
    -liphlpapi -ladvapi32

if errorlevel 1 (
    echo [ERROR] DLL compilation failed
    exit /b 1
)
echo [OK] HWIDChanger.dll

echo.
echo [3/3] Building HWIDTest.exe (API verification)...
"%GCC%" -Os -s -static -ffunction-sections -fdata-sections "-Wl,--gc-sections" ^
    -o "%BIN%\HWIDTest.exe" ^
    hwid_test.c ^
    -liphlpapi -ladvapi32

if errorlevel 1 (
    echo [WARN] Test program compilation failed
) else (
    echo [OK] HWIDTest.exe
)

echo.
echo ======================================
echo  Build Complete!
echo ======================================
dir /b "%BIN%\HWIDChanger.*"
echo.
for %%F in ("%BIN%\HWIDChanger.exe" "%BIN%\HWIDChanger.dll") do (
    for %%A in (%%F) do echo %%~nxF: %%~zA bytes
)

endlocal
