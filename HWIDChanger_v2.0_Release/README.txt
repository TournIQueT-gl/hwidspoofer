================================================================================
                        HWID Changer v2.0
                    全系统硬件ID伪装工具
================================================================================

【文件说明】
  HWIDChanger.exe   - 主程序
  HWIDChanger.dll   - Hook注入模块（自动使用，无需手动操作）
  HWIDTest.exe      - 硬件信息查看工具
  config_example.cfg - 配置文件示例

================================================================================
                            快速使用
================================================================================

【一键全系统伪装】
  .\HWIDChanger.exe -r --full-usermode

【恢复原始值】
  .\HWIDChanger.exe --restore-registry
  .\HWIDChanger.exe --disable-appinit

【查看当前硬件信息】
  .\HWIDTest.exe

【查看帮助】
  .\HWIDChanger.exe --help

================================================================================
                            命令参考
================================================================================

基础命令:
  -r, --random           生成随机硬件ID
  -s, --save <文件>      保存配置到文件
  -c, --config <文件>    加载配置文件
  -h, --help             显示帮助

伪装命令:
  --full-usermode        全系统用户态伪装（推荐）
  --registry-global      仅修改注册表
  --enable-appinit       启用AppInit DLL注入
  -n, --name <程序名>    注入到指定程序
  -p, --pid <PID>        注入到指定进程ID

恢复命令:
  --restore-registry     恢复注册表原始值
  --disable-appinit      禁用AppInit注入

================================================================================
                            使用示例
================================================================================

示例1: 一键伪装
  .\HWIDChanger.exe -r --full-usermode

示例2: 使用自定义配置
  .\HWIDChanger.exe -c config_example.cfg --full-usermode

示例3: 只修改注册表
  .\HWIDChanger.exe -r --registry-global

示例4: 注入到指定程序
  .\HWIDChanger.exe -r -n game.exe

示例5: 生成配置并保存
  .\HWIDChanger.exe -r -s my_config.cfg

================================================================================
                            伪装内容
================================================================================

【注册表伪装】(--registry-global 或 --full-usermode)
  - MachineGuid     机器唯一标识
  - ProductId       Windows产品ID
  - ComputerName    计算机名
  - HwProfileGuid   硬件配置文件
  - MAC地址         网卡物理地址（注册表级别）

【API Hook伪装】(进程注入 或 AppInit)
  - 磁盘序列号      DeviceIoControl
  - 卷序列号        GetVolumeInformation
  - MAC地址         GetAdaptersInfo
  - SMBIOS数据      GetSystemFirmwareTable
  - 注册表值        RegQueryValueEx

================================================================================
                            注意事项
================================================================================

1. 需要以管理员身份运行
   - 右键选择"以管理员身份运行"
   - 或在管理员PowerShell/CMD中执行

2. 备份自动保存
   - 首次修改时自动备份到 hwid_backup.dat
   - 使用 --restore-registry 可恢复

3. 部分修改需要重启
   - ComputerName 修改后需要重启生效
   - MAC地址修改可能需要禁用/启用网卡

4. PowerShell 执行注意
   - 在 PowerShell 中需要加 .\ 前缀
   - 例如: .\HWIDChanger.exe --help

5. HWIDChanger.dll 说明
   - 此文件必须与主程序在同一目录
   - 用于 AppInit 自动注入功能
   - 无需手动操作

================================================================================
                            验证效果
================================================================================

方法1: 使用 HWIDTest.exe
  .\HWIDTest.exe

方法2: 使用系统命令
  reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid

方法3: 使用 PowerShell
  (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid

================================================================================
                          版本: 2.0
================================================================================

