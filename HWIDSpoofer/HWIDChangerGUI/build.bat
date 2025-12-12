@echo off
chcp 65001 >nul
echo ========================================
echo   HWID Changer GUI - 构建脚本
echo ========================================
echo.

:: 查找MSBuild
set "MSBUILD="
for %%i in (
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\MSBuild.exe"
    "C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
) do (
    if exist "%%~i" (
        set "MSBUILD=%%~i"
        goto :found
    )
)

echo [错误] 未找到 MSBuild.exe
echo 请安装 Visual Studio 或 .NET Framework SDK
pause
exit /b 1

:found
echo [信息] 使用 MSBuild: %MSBUILD%
echo.

:: 如果没有图标，创建一个空的占位符
if not exist "app.ico" (
    echo [警告] 未找到 app.ico，将使用默认图标
    :: 使用PowerShell创建一个简单的图标
    powershell -Command "$icon = [System.Drawing.Icon]::ExtractAssociatedIcon('%SystemRoot%\System32\shell32.dll'); $fs = [System.IO.File]::Create('app.ico'); $icon.Save($fs); $fs.Close()" 2>nul
    if not exist "app.ico" (
        echo [信息] 正在下载默认图标...
        copy nul app.ico >nul 2>&1
    )
)

:: 编译Release版本
echo [步骤 1/2] 编译 Release 版本...
"%MSBUILD%" HWIDChangerGUI.csproj /p:Configuration=Release /p:Platform=AnyCPU /v:minimal /nologo

if %ERRORLEVEL% neq 0 (
    echo [错误] 编译失败！
    pause
    exit /b 1
)

echo [步骤 2/2] 复制依赖文件...

:: 创建输出目录
if not exist "bin\Release\HWIDChanger" mkdir "bin\Release\HWIDChanger"

:: 复制GUI程序
copy /y "bin\Release\HWIDChangerGUI.exe" "bin\Release\HWIDChanger\" >nul
copy /y "bin\Release\HWIDChangerGUI.exe.config" "bin\Release\HWIDChanger\" >nul 2>&1

:: 复制核心程序（如果存在）
set "CORE_BIN=..\HWIDChanger\bin"
if exist "%CORE_BIN%\HWIDChanger.exe" (
    copy /y "%CORE_BIN%\HWIDChanger.exe" "bin\Release\HWIDChanger\" >nul
    echo [信息] 已复制 HWIDChanger.exe
)
if exist "%CORE_BIN%\HWIDChanger.dll" (
    copy /y "%CORE_BIN%\HWIDChanger.dll" "bin\Release\HWIDChanger\" >nul
    echo [信息] 已复制 HWIDChanger.dll
)
if exist "%CORE_BIN%\HWIDTest.exe" (
    copy /y "%CORE_BIN%\HWIDTest.exe" "bin\Release\HWIDChanger\" >nul
    echo [信息] 已复制 HWIDTest.exe
)
if exist "%CORE_BIN%\README.md" (
    copy /y "%CORE_BIN%\README.md" "bin\Release\HWIDChanger\" >nul
    echo [信息] 已复制 README.md
)

echo.
echo ========================================
echo   构建完成！
echo ========================================
echo.
echo 输出目录: %CD%\bin\Release\HWIDChanger
echo.
dir /b "bin\Release\HWIDChanger"
echo.
pause

