@echo off
:: HWID Changer 驱动安装脚本
:: 需要管理员权限运行

net session >nul 2>&1
if %errorlevel% neq 0 (
    echo 请以管理员身份运行此脚本！
    pause
    exit /b 1
)

echo.
echo ═══════════════════════════════════════════════════════════════
echo              HWID Changer Driver Installer
echo ═══════════════════════════════════════════════════════════════
echo.

:: 检查测试模式
bcdedit /enum {current} 2>nul | findstr "testsigning.*Yes" >nul
if %errorlevel% neq 0 (
    echo [警告] 测试签名未启用！
    echo.
    echo 要使用未签名驱动，需要启用测试模式：
    echo   1. 以管理员身份运行: bcdedit /set testsigning on
    echo   2. 重启电脑
    echo.
    set /p C="是否继续？(y/n): "
    if /i not "!C!"=="y" exit /b 0
)

:: 检查驱动文件
if not exist "HWIDChanger.sys" (
    echo [错误] 找不到驱动文件: HWIDChanger.sys
    echo 请先编译驱动程序
    pause
    exit /b 1
)

:: 停止并删除旧服务
echo [1/3] 清理旧服务...
sc stop HWIDChanger >nul 2>&1
sc delete HWIDChanger >nul 2>&1
timeout /t 1 >nul

:: 安装新服务
echo [2/3] 安装驱动...
set DRV_PATH=%~dp0HWIDChanger.sys
sc create HWIDChanger type=kernel start=demand binPath="%DRV_PATH%" displayname="HWID Changer"

if %errorlevel% neq 0 (
    echo [错误] 服务创建失败
    pause
    exit /b 1
)

:: 启动服务
echo [3/3] 启动驱动...
sc start HWIDChanger

if %errorlevel% neq 0 (
    echo.
    echo [错误] 驱动启动失败！
    echo 可能原因：
    echo   1. 测试签名未启用
    echo   2. 驱动没有正确签名
    echo   3. Windows安全中心阻止
    echo.
    pause
    exit /b 1
)

echo.
echo ═══════════════════════════════════════════════════════════════
echo                    安装成功！
echo ═══════════════════════════════════════════════════════════════
echo.
sc query HWIDChanger | findstr "STATE"
echo.
pause

