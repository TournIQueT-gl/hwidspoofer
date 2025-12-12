@echo off
setlocal

echo ============================================
echo    HWID Spoofer Driver Uninstaller
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

:: 停止服务
echo [1/2] Stopping driver...
sc stop HWIDSpoofer
timeout /t 2 >nul

:: 删除服务
echo [2/2] Removing driver...
sc delete HWIDSpoofer

if %errorlevel%==0 (
    echo.
    echo ============================================
    echo    Driver Uninstalled Successfully!
    echo ============================================
) else (
    echo.
    echo [WARNING] Driver may not have been fully removed.
    echo Try restarting your computer.
)

echo.
pause

