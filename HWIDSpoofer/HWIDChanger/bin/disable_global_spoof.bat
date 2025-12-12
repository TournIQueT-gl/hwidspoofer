@echo off
REM ============================================================
REM  HWID Spoofer - 禁用全系统伪装
REM  需要以管理员身份运行
REM ============================================================

title HWID Spoofer - Disable

echo.
echo  ============================================
echo   HWID Spoofer - Disabling Global Spoof
echo  ============================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo  [ERROR] This script requires Administrator privileges!
    pause
    exit /b 1
)

echo  [1/3] Stopping kernel driver...

sc stop HWIDSpoofer >nul 2>&1
sc delete HWIDSpoofer >nul 2>&1

echo  [OK] Driver stopped

echo  [2/3] Restoring registry...

if exist HWIDChanger.exe (
    HWIDChanger.exe --restore-registry -q
    echo  [OK] Registry restored
) else (
    echo  [WARN] Cannot restore registry automatically
)

echo  [3/3] Complete!
echo.
echo  ============================================
echo   Status: Global spoofing DISABLED
echo  ============================================
echo.
echo  Some changes may require a reboot to take effect.
echo.

pause

