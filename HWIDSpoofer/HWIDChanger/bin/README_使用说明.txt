╔══════════════════════════════════════════════════════════════════╗
║              HWID Changer 全系统伪装使用说明                     ║
╚══════════════════════════════════════════════════════════════════╝

============================================================
快速开始
============================================================

方式一：一键启动（推荐）
------------------------
  1. 右键 full_spoof.bat -> "以管理员身份运行"
  2. 选择 [1] 应用全系统伪装
  3. 完成！

方式二：命令行（推荐）
------------------------
  # 一键全系统伪装（注册表 + AppInit自动注入）
  HWIDChanger.exe -r --full-usermode

  # 恢复原始值
  HWIDChanger.exe --restore-registry
  HWIDChanger.exe --disable-appinit


============================================================
三种伪装模式详解
============================================================

┌─────────────────────────────────────────────────────────┐
│ 模式1: 注册表修改 (--registry-global)                   │
├─────────────────────────────────────────────────────────┤
│ 影响范围: 所有程序                                      │
│ 需要权限: 管理员                                        │
│ 持久性:   重启后保持                                    │
│                                                         │
│ 伪装内容:                                               │
│   ✓ MachineGuid (机器唯一标识)                          │
│   ✓ ProductId (Windows产品ID)                           │
│   ✓ ComputerName (计算机名)                             │
│   ✓ HwProfileGuid (硬件配置文件)                        │
│   ✓ 网卡MAC地址 (注册表级别)                            │
│                                                         │
│ 命令: HWIDChanger.exe -c config.cfg --registry-global   │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ 模式2: 进程注入 (-n 程序名)                             │
├─────────────────────────────────────────────────────────┤
│ 影响范围: 仅目标程序                                    │
│ 需要权限: 管理员                                        │
│ 持久性:   程序关闭后失效                                │
│                                                         │
│ 伪装内容:                                               │
│   ✓ 磁盘序列号 (DeviceIoControl)                        │
│   ✓ MAC地址 (GetAdaptersInfo)                           │
│   ✓ SMBIOS数据 (GetSystemFirmwareTable)                 │
│   ✓ 卷序列号 (GetVolumeInformation)                     │
│   ✓ 注册表值 (RegQueryValueEx)                          │
│                                                         │
│ 命令: HWIDChanger.exe -c config.cfg -n game.exe         │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ 模式3: 全系统用户态 (--full-usermode) 【推荐】          │
├─────────────────────────────────────────────────────────┤
│ 影响范围: 所有程序                                      │
│ 需要权限: 管理员                                        │
│ 持久性:   重启后注册表保持，AppInit需重新启用           │
│                                                         │
│ 伪装内容:                                               │
│   ✓ 注册表修改 (所有程序生效)                           │
│   ✓ AppInit自动注入 (新启动的GUI程序自动Hook)           │
│                                                         │
│ 命令: HWIDChanger.exe -r --full-usermode                │
│ 恢复: HWIDChanger.exe --restore-registry                │
│       HWIDChanger.exe --disable-appinit                 │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ 模式4: 内核驱动全局 (--enable-global)                   │
├─────────────────────────────────────────────────────────┤
│ 影响范围: 所有程序的所有API                             │
│ 需要权限: 管理员 + 测试签名模式                         │
│ 持久性:   驱动卸载后失效                                │
│                                                         │
│ 伪装内容: 全部硬件ID                                    │
│                                                         │
│ 前置条件:                                               │
│   1. bcdedit /set testsigning on                        │
│   2. 重启电脑                                           │
│   3. 将 HWIDSpoofer.sys 放到同目录                      │
│                                                         │
│ 命令: HWIDChanger.exe -c config.cfg --enable-global     │
└─────────────────────────────────────────────────────────┘


============================================================
常用命令
============================================================

生成随机配置:
  HWIDChanger.exe -r -s my_config.cfg

查看帮助:
  HWIDChanger.exe --help

应用注册表伪装:
  HWIDChanger.exe -c my_config.cfg --registry-global

恢复注册表:
  HWIDChanger.exe --restore-registry

注入到程序:
  HWIDChanger.exe -c my_config.cfg -n 目标程序.exe

启用全局模式(需要驱动):
  HWIDChanger.exe -c my_config.cfg --enable-global

禁用全局模式:
  HWIDChanger.exe --disable-global

全系统用户态伪装（推荐）:
  HWIDChanger.exe -r --full-usermode

禁用AppInit注入:
  HWIDChanger.exe --disable-appinit


============================================================
验证伪装效果
============================================================

方式1: 使用 HWIDTest.exe
  HWIDTest.exe

方式2: 使用系统命令
  reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid

方式3: 使用 PowerShell
  (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid


============================================================
注意事项
============================================================

1. 首次使用前建议备份
   - 备份文件自动保存为 hwid_backup.dat
   - 恢复命令: HWIDChanger.exe --restore-registry

2. 部分修改需要重启生效
   - ComputerName 修改需要重启
   - MAC地址修改可能需要禁用/启用网卡

3. 关于测试签名模式
   - 内核驱动需要测试签名模式
   - 启用: bcdedit /set testsigning on
   - 禁用: bcdedit /set testsigning off
   - 注意: 某些反作弊软件会检测测试签名模式

4. 安全建议
   - 在虚拟机中测试
   - 定期恢复原始值
   - 不要用于非法用途


============================================================
文件说明
============================================================

核心文件:
  HWIDChanger.exe    - 主程序
  HWIDChanger.dll    - 注入用DLL（AppInit自动加载）

工具:
  HWIDTest.exe       - 硬件信息查看工具
  HWIDSelfTest.exe   - 自动化测试工具

脚本:
  full_spoof.bat     - 一键启动脚本（交互式菜单）
  enable_global_spoof.bat  - 启用全局伪装
  disable_global_spoof.bat - 禁用全局伪装

数据文件:
  hwid_backup.dat    - 注册表备份文件
  hwid_current.cfg   - 当前使用的配置

可选:
  HWIDSpoofer.sys    - 内核驱动（需要WDK编译）


============================================================
技术支持
============================================================

如遇问题，请检查:
1. 是否以管理员身份运行
2. 杀毒软件是否拦截
3. 配置文件格式是否正确


