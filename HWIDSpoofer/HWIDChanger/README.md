# HWID Changer v2.0

单文件硬件信息伪装工具，支持用户态Hook和内核驱动两种模式。

## 特点

- ✅ **单一EXE** - 无外部依赖
- ✅ **体积小** - 约50KB
- ✅ **双模式** - 用户态 + 内核驱动
- ✅ **全覆盖** - 磁盘/BIOS/主板/MAC/UUID等

## 快速开始

```bash
# 交互模式
HWIDChanger.exe

# 命令行模式
HWIDChanger.exe -r -a      # 随机生成并应用
HWIDChanger.exe -n app.exe # 注入到指定进程
HWIDChanger.exe -d         # 加载内核驱动（更彻底）
```

## 命令行参数

| 参数 | 说明 |
|------|------|
| `-r, --random` | 生成随机硬件ID |
| `-a, --apply` | 应用伪装 |
| `-d, --driver` | 加载内核驱动 |
| `-p, --pid <PID>` | 注入到指定进程 |
| `-n, --name <名称>` | 注入到指定进程名 |
| `-c, --config <文件>` | 加载配置文件 |
| `-s, --save <文件>` | 保存配置文件 |
| `-h, --help` | 显示帮助 |

## 构建

```bash
# 方法1: 使用构建脚本
build.bat

# 方法2: 直接编译
cl /O2 /MT main.c hwid_core.c hwid_hooks.c /Fe:HWIDChanger.exe kernel32.lib advapi32.lib iphlpapi.lib
```

## 配置文件格式

```ini
# hwid.cfg
flags = 0xFFFF

disk_serial = "WD-WMC4N0123456"
volume_serial = 0x12345678

bios_serial = "BIOS123456789"
bios_vendor = "American Megatrends Inc."

board_serial = "MB12345678901"
board_product = "B460M-PLUS"

system_uuid = "12345678-1234-4321-ABCD-123456789ABC"
machine_guid = "87654321-4321-1234-DCBA-CBA987654321"

mac_address = "00:1A:2B:3C:4D:5E"
cpu_id = "BFEBFBFF000906ED"
```

## 支持的硬件信息

| 类型 | 用户态 | 内核态 |
|------|--------|--------|
| 磁盘序列号 | ✅ | ✅ |
| 卷序列号 | ✅ | ✅ |
| BIOS序列号 | ✅ | ✅ |
| 主板序列号 | ✅ | ✅ |
| 系统UUID | ✅ | ✅ |
| MachineGuid | ✅ | ❌ |
| MAC地址 | ✅ | ✅ |
| CPU ID | ⚠️ | ✅ |
| SMBIOS | ✅ | ✅ |

## 使用内核驱动

```bash
# 1. 启用测试模式（管理员权限）
bcdedit /set testsigning on

# 2. 重启电脑

# 3. 安装驱动
cd driver
install.bat

# 4. 使用
HWIDChanger.exe -d
```

## 文件结构

```
HWIDChanger/
├── main.c              # 主程序
├── hwid_core.c         # 核心功能
├── hwid_hooks.c        # Hook实现
├── hwid_changer.h      # 头文件
├── build.bat           # 构建脚本
├── driver/             # 内核驱动
│   ├── hwid_driver.c
│   ├── install.bat
│   └── makefile
└── README.md
```

## 注意事项

⚠️ **仅供开发测试使用**

- 需要管理员权限
- 内核驱动需要测试模式
- 某些反作弊软件可能检测

