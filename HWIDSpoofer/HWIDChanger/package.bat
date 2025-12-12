@echo off
chcp 65001 >nul
echo.
echo ============================================
echo   HWID Changer v2.0 - Release Package
echo ============================================
echo.

set RELEASE_DIR=..\..\..\HWIDChanger_v2.0_Release

:: 创建发布目录
if exist "%RELEASE_DIR%" rmdir /s /q "%RELEASE_DIR%"
mkdir "%RELEASE_DIR%"

echo [1/3] Copying executable files...
copy bin\HWIDChanger.exe "%RELEASE_DIR%\" >nul
copy bin\HWIDChanger.dll "%RELEASE_DIR%\" >nul
copy bin\HWIDTest.exe "%RELEASE_DIR%\" >nul

echo [2/3] Copying launcher script...
copy bin\full_spoof.bat "%RELEASE_DIR%\启动.bat" >nul

echo [3/3] Creating config and readme...
bin\HWIDChanger.exe -r -s "%RELEASE_DIR%\config_example.cfg" >nul 2>&1

:: 创建简洁的README
(
echo HWID Changer v2.0
echo =================
echo.
echo 【快速使用】
echo   右键"启动.bat" -^> 以管理员身份运行 -^> 选择 [1]
echo.
echo 【命令行使用】
echo   一键伪装:     HWIDChanger.exe -r --full-usermode
echo   恢复原始值:   HWIDChanger.exe --restore-registry
echo                 HWIDChanger.exe --disable-appinit
echo   查看帮助:     HWIDChanger.exe --help
echo   查看硬件ID:   HWIDTest.exe
echo.
echo 【文件说明】
echo   HWIDChanger.exe  - 主程序
echo   HWIDChanger.dll  - Hook注入模块
echo   HWIDTest.exe     - 硬件信息查看工具
echo   启动.bat         - 一键启动脚本
echo   config_example.cfg - 配置文件示例
echo.
echo 【注意事项】
echo   1. 需要以管理员身份运行
echo   2. 首次使用会自动备份原始值
echo   3. 使用 --restore-registry 可恢复
echo.
) > "%RELEASE_DIR%\README.txt"

echo.
echo ============================================
echo   Package Complete!
echo ============================================
echo.
echo 发布目录: %RELEASE_DIR%
echo.
echo 文件列表:
dir /b "%RELEASE_DIR%"
echo.
echo 文件大小:
for %%F in ("%RELEASE_DIR%\*.*") do echo   %%~nxF  -  %%~zF bytes
echo.
pause

