@echo off
REM ============================================================
REM  HWID Spoofer - 全系统级别伪装启动脚本
REM  需要以管理员身份运行
REM ============================================================

title HWID Spoofer - Global Mode

echo.
echo  ============================================
echo   HWID Spoofer - Global System Spoofing
echo  ============================================
echo.

REM 检查管理员权限
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo  [ERROR] This script requires Administrator privileges!
    echo  Right-click and select "Run as administrator"
    echo.
    pause
    exit /b 1
)

echo  [1/5] Checking test signing mode...

REM 检查测试模式
bcdedit /enum | findstr /i "testsigning.*Yes" >nul 2>&1
if %errorLevel% neq 0 (
    echo.
    echo  [!] Test signing is NOT enabled.
    echo  The kernel driver requires test signing mode.
    echo.
    set /p ENABLE_TEST="  Enable test signing and reboot? [Y/N]: "
    if /i "%ENABLE_TEST%"=="Y" (
        bcdedit /set testsigning on
        echo.
        echo  [OK] Test signing enabled. Please REBOOT and run this script again.
        pause
        exit /b 0
    ) else (
        echo  [!] Continuing without driver (limited functionality)
    )
)

echo  [2/5] Generating random HWID configuration...

REM 生成随机配置
if exist HWIDChanger.exe (
    HWIDChanger.exe -r -s hwid_global.cfg -q
    echo  [OK] Configuration saved to hwid_global.cfg
) else (
    echo  [ERROR] HWIDChanger.exe not found!
    pause
    exit /b 1
)

echo  [3/5] Applying registry modifications...

REM 应用注册表修改
HWIDChanger.exe -c hwid_global.cfg --registry-global -q
if %errorLevel% equ 0 (
    echo  [OK] Registry modified
) else (
    echo  [WARN] Some registry modifications failed
)

echo  [4/5] Loading kernel driver...

REM 检查驱动文件
if not exist HWIDSpoofer.sys (
    echo  [WARN] Driver file not found, skipping driver load
    goto :skip_driver
)

REM 停止旧驱动
sc stop HWIDSpoofer >nul 2>&1
sc delete HWIDSpoofer >nul 2>&1
timeout /t 1 >nul

REM 安装新驱动
sc create HWIDSpoofer type=kernel binPath="%~dp0HWIDSpoofer.sys" start=demand >nul 2>&1
sc start HWIDSpoofer >nul 2>&1

if %errorLevel% equ 0 (
    echo  [OK] Driver loaded successfully
    
    REM 配置驱动并启用全局模式
    HWIDChanger.exe -c hwid_global.cfg -d --enable-global -q
    echo  [OK] Global interception mode enabled
) else (
    echo  [WARN] Failed to load driver
)

:skip_driver

echo  [5/5] Complete!
echo.
echo  ============================================
echo   Status: Global HWID spoofing is ACTIVE
echo  ============================================
echo.
echo  All programs will now see spoofed hardware IDs.
echo  Configuration file: hwid_global.cfg
echo.
echo  To disable, run: disable_global_spoof.bat
echo  Or reboot your computer.
echo.

pause

