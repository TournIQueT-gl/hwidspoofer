@echo off
chcp 65001 >nul
echo ========================================
echo   HWID Changer - MSI 安装包创建
echo ========================================
echo.

:: 检查是否已编译
if not exist "bin\Release\HWIDChanger\HWIDChangerGUI.exe" (
    echo [错误] 请先运行 build.bat 编译项目
    pause
    exit /b 1
)

:: 查找Inno Setup
set "INNO="
for %%i in (
    "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
    "%ProgramFiles%\Inno Setup 6\ISCC.exe"
    "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    "C:\Program Files\Inno Setup 6\ISCC.exe"
) do (
    if exist "%%~i" (
        set "INNO=%%~i"
        goto :found_inno
    )
)

echo [警告] 未找到 Inno Setup 6
echo.
echo 请从以下地址下载安装 Inno Setup:
echo https://jrsoftware.org/isdl.php
echo.
echo 或者手动复制 bin\Release\HWIDChanger 目录分发
pause
exit /b 1

:found_inno
echo [信息] 使用 Inno Setup: %INNO%
echo.

:: 创建Inno Setup脚本
echo [步骤 1/2] 生成安装脚本...

(
echo ; HWID Changer 安装脚本
echo ; 自动生成 - %date% %time%
echo.
echo #define MyAppName "HWID Changer"
echo #define MyAppVersion "2.0"
echo #define MyAppPublisher "HWID Tools"
echo #define MyAppExeName "HWIDChangerGUI.exe"
echo.
echo [Setup]
echo AppId={{A1B2C3D4-E5F6-7890-HWID-CHANGER2024}
echo AppName={#MyAppName}
echo AppVersion={#MyAppVersion}
echo AppPublisher={#MyAppPublisher}
echo DefaultDirName={autopf}\{#MyAppName}
echo DefaultGroupName={#MyAppName}
echo AllowNoIcons=yes
echo PrivilegesRequired=admin
echo OutputDir=installer
echo OutputBaseFilename=HWIDChanger_Setup_v2.0
echo SetupIconFile=app.ico
echo Compression=lzma2/ultra64
echo SolidCompression=yes
echo WizardStyle=modern
echo.
echo [Languages]
echo Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
echo Name: "english"; MessagesFile: "compiler:Default.isl"
echo.
echo [Tasks]
echo Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
echo Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
echo.
echo [Files]
echo Source: "bin\Release\HWIDChanger\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs
echo.
echo [Icons]
echo Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
echo Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
echo Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
echo.
echo [Run]
echo Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent runascurrentuser
) > installer.iss

:: 检查图标
if not exist "app.ico" (
    echo [警告] 未找到 app.ico，使用系统默认图标
    powershell -Command "(Get-Content installer.iss) -replace 'SetupIconFile=app.ico', '; SetupIconFile=app.ico' | Set-Content installer.iss"
)

:: 编译安装包
echo [步骤 2/2] 编译安装包...
"%INNO%" installer.iss

if %ERRORLEVEL% neq 0 (
    echo [错误] 安装包创建失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo   安装包创建完成！
echo ========================================
echo.
echo 输出文件: %CD%\installer\HWIDChanger_Setup_v2.0.exe
echo.
pause

