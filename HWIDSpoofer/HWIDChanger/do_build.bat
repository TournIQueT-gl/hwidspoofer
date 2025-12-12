@echo off
chcp 65001 >nul
echo.
echo ══════════════════════════════════════════════
echo         HWID Changer - Building...
echo ══════════════════════════════════════════════
echo.

call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=amd64 -no_logo

cd /d "%~dp0"
if not exist bin mkdir bin

echo Compiling x64...
cl /nologo /O2 /MT /W3 /utf-8 ^
   /D "WIN32_LEAN_AND_MEAN" /D "_CRT_SECURE_NO_WARNINGS" /D "NDEBUG" ^
   main.c hwid_core.c hwid_hooks.c ^
   /Fe"bin\HWIDChanger.exe" ^
   /link /OPT:REF kernel32.lib advapi32.lib iphlpapi.lib user32.lib

if exist bin\HWIDChanger.exe (
    echo.
    echo [SUCCESS] Build complete!
    for %%F in (bin\HWIDChanger.exe) do echo Size: %%~zF bytes
) else (
    echo.
    echo [FAILED] Build failed!
)

del *.obj 2>nul
echo.

