@echo off
chcp 65001 >nul
echo.
echo ════════════════════════════════════════════════════════════
echo                 HWID Changer 编译中...
echo ════════════════════════════════════════════════════════════
echo.

:: 初始化VS环境
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

if not exist "bin" mkdir bin

echo [1/2] 编译 x64 版本...
cl /nologo /O2 /GL /GS- /Gy /MT /W3 ^
   /D "WIN32_LEAN_AND_MEAN" /D "NOMINMAX" /D "_CRT_SECURE_NO_WARNINGS" /D "NDEBUG" ^
   main.c hwid_core.c hwid_hooks.c registry_spoof.c ^
   /Fe"bin\HWIDChanger.exe" ^
   /link /LTCG /OPT:REF /OPT:ICF kernel32.lib advapi32.lib iphlpapi.lib user32.lib

if %errorlevel% neq 0 (
    echo [错误] 编译失败!
    exit /b 1
)

del *.obj 2>nul

echo.
echo ════════════════════════════════════════════════════════════
echo                      编译完成!
echo ════════════════════════════════════════════════════════════
echo.
echo 输出文件: bin\HWIDChanger.exe
for %%F in (bin\HWIDChanger.exe) do echo 文件大小: %%~zF 字节
echo.

