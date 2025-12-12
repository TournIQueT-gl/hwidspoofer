@echo off
chcp 65001 >nul
setlocal

echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║       HWID Spoofer - Full System Spoofing                ║
echo ║       全系统硬件ID伪装工具                               ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

:: 检查管理员权限
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] 需要管理员权限！请右键选择"以管理员身份运行"
    pause
    exit /b 1
)

echo [INFO] 运行模式：管理员
echo.

:: ============================================================
:: 菜单
:: ============================================================
:menu
echo ════════════════════════════════════════════════════════════
echo   请选择操作：
echo ════════════════════════════════════════════════════════════
echo.
echo   [1] 应用全系统伪装 (注册表 + 尝试内核驱动)
echo   [2] 仅修改注册表 (无需驱动)
echo   [3] 注入到指定程序
echo   [4] 恢复原始值
echo   [5] 查看当前硬件ID
echo   [6] 生成新的随机配置
echo   [0] 退出
echo.
echo ════════════════════════════════════════════════════════════
set /p choice="请输入选项 [0-6]: "

if "%choice%"=="1" goto full_spoof
if "%choice%"=="2" goto registry_only
if "%choice%"=="3" goto inject
if "%choice%"=="4" goto restore
if "%choice%"=="5" goto show_info
if "%choice%"=="6" goto generate
if "%choice%"=="0" goto exit
goto menu

:: ============================================================
:: 全系统伪装
:: ============================================================
:full_spoof
echo.
echo [INFO] 应用全系统用户态伪装...
echo.
HWIDChanger.exe -r -s hwid_current.cfg --full-usermode
echo.

echo [步骤 4/4] 验证伪装效果...
echo.
echo   当前 MachineGuid:
reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid | findstr "MachineGuid"
echo.
echo ════════════════════════════════════════════════════════════
echo   全系统伪装已应用！
echo   - 注册表已修改 (所有程序生效)
echo   - 配置文件: hwid_current.cfg
echo ════════════════════════════════════════════════════════════
echo.
pause
goto menu

:: ============================================================
:: 仅注册表
:: ============================================================
:registry_only
echo.
if not exist hwid_current.cfg (
    echo [INFO] 生成新配置...
    HWIDChanger.exe -r -s hwid_current.cfg
)
echo [INFO] 应用注册表修改...
HWIDChanger.exe -c hwid_current.cfg --registry-global
echo.
echo [OK] 注册表修改完成！
pause
goto menu

:: ============================================================
:: 注入程序
:: ============================================================
:inject
echo.
set /p procname="请输入目标程序名称 (如 game.exe): "
if "%procname%"=="" goto menu

if not exist hwid_current.cfg (
    HWIDChanger.exe -r -s hwid_current.cfg
)
echo [INFO] 注入到 %procname%...
HWIDChanger.exe -c hwid_current.cfg -n %procname%
echo.
pause
goto menu

:: ============================================================
:: 恢复
:: ============================================================
:restore
echo.
echo [INFO] 恢复注册表原始值...
HWIDChanger.exe --restore-registry
echo.
echo [INFO] 禁用AppInit注入...
HWIDChanger.exe --disable-appinit
echo.
echo [INFO] 停止内核驱动 (如果有)...
sc stop HWIDSpoofer >nul 2>&1
sc delete HWIDSpoofer >nul 2>&1
echo.
echo [OK] 已恢复原始值
pause
goto menu

:: ============================================================
:: 显示信息
:: ============================================================
:show_info
echo.
echo ════════════════════════════════════════════════════════════
echo   当前系统硬件信息
echo ════════════════════════════════════════════════════════════
echo.
HWIDTest.exe 2>nul || (
    echo [使用注册表查询]
    echo.
    echo MachineGuid:
    reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid | findstr "REG_SZ"
    echo.
    echo ProductId:
    reg query "HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion" /v ProductId | findstr "REG_SZ"
    echo.
    echo ComputerName:
    echo %COMPUTERNAME%
)
echo.
pause
goto menu

:: ============================================================
:: 生成配置
:: ============================================================
:generate
echo.
echo [INFO] 生成新的随机配置...
HWIDChanger.exe -r -s hwid_current.cfg
echo [OK] 新配置已保存到 hwid_current.cfg
echo.
echo 配置内容:
type hwid_current.cfg | findstr /V "^#" | findstr /V "^$"
echo.
pause
goto menu

:: ============================================================
:: 退出
:: ============================================================
:exit
echo.
echo 再见！
exit /b 0

