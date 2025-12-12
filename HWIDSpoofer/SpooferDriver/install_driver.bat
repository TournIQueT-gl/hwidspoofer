@echo off
setlocal

echo ============================================
echo    HWID Spoofer Driver Installer
echo ============================================
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] This script requires Administrator privileges!
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: 检查测试模式
bcdedit /enum {current} | findstr "testsigning.*Yes" >nul 2>&1
if %errorlevel% neq 0 (
    echo [WARNING] Test signing is not enabled!
    echo.
    echo To use unsigned drivers, you need to enable test signing mode:
    echo   1. Run: bcdedit /set testsigning on
    echo   2. Restart your computer
    echo.
    echo Note: Test mode will show a watermark on your desktop.
    echo.
    set /p CONTINUE="Continue anyway? (y/n): "
    if /i not "!CONTINUE!"=="y" exit /b 0
)

:: 检查驱动文件
if not exist "HWIDSpoofer.sys" (
    echo [ERROR] Driver file not found: HWIDSpoofer.sys
    echo Please build the driver first using Visual Studio + WDK
    pause
    exit /b 1
)

:: 停止已有服务
echo [1/3] Stopping existing service...
sc stop HWIDSpoofer >nul 2>&1
sc delete HWIDSpoofer >nul 2>&1
timeout /t 2 >nul

:: 复制驱动文件
echo [2/3] Installing driver...
set DRIVER_PATH=%CD%\HWIDSpoofer.sys
echo Driver path: %DRIVER_PATH%

:: 创建服务
sc create HWIDSpoofer type=kernel start=demand binPath="%DRIVER_PATH%" displayname="HWID Spoofer Driver"
if %errorlevel% neq 0 (
    echo [ERROR] Failed to create service
    pause
    exit /b 1
)

:: 启动服务
echo [3/3] Starting driver...
sc start HWIDSpoofer
if %errorlevel% neq 0 (
    echo [ERROR] Failed to start driver
    echo.
    echo Possible reasons:
    echo   1. Test signing is not enabled
    echo   2. Driver is not properly signed
    echo   3. Windows blocked the driver
    echo.
    echo Check Event Viewer for more details.
    pause
    exit /b 1
)

echo.
echo ============================================
echo    Driver Installed Successfully!
echo ============================================
echo.
echo Status:
sc query HWIDSpoofer | findstr "STATE"
echo.

pause

