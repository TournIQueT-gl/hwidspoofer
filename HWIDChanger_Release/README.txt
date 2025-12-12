HWID Changer v2.0
=================

硬件信息伪装工具，用于开发测试。

使用方法
--------

1. 交互模式（推荐）:
   双击运行 HWIDChanger.exe
   按数字键选择功能

2. 命令行模式:
   HWIDChanger.exe -r           生成随机硬件ID
   HWIDChanger.exe -r -a        生成并应用到当前进程
   HWIDChanger.exe -r -s xx.cfg 生成并保存到配置文件
   HWIDChanger.exe -c xx.cfg    加载配置文件
   HWIDChanger.exe -n app.exe   注入到指定进程
   HWIDChanger.exe -p 1234      注入到指定PID
   HWIDChanger.exe -d           加载内核驱动(需管理员+测试模式)
   HWIDChanger.exe -h           显示帮助

支持伪装的硬件信息
------------------
- 磁盘序列号 / 卷序列号
- BIOS序列号 / 主板序列号  
- 系统UUID / MachineGuid
- MAC地址
- CPU ID

注意事项
--------
1. 需要管理员权限运行
2. 杀毒软件可能拦截，请加入白名单
3. 驱动模式需先启用测试签名:
   bcdedit /set testsigning on
   然后重启电脑

文件说明
--------
HWIDChanger.exe    - 主程序 (40KB)
config_example.cfg - 示例配置文件
README.txt         - 本说明文件

