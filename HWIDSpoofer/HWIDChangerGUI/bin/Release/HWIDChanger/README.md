# HWID Changer v2.0

**全系统硬件ID伪装工具** - 用户态实现，无需内核驱动

---

## 📦 文件说明

| 文件 | 说明 |
|------|------|
| `HWIDChanger.exe` | 主程序 (47KB) |
| `HWIDChanger.dll` | Hook注入DLL (21KB) |
| `HWIDTest.exe` | 硬件信息查看工具 |
| `HWIDSelfTest.exe` | 自动化测试工具 |
| `full_spoof.bat` | 一键启动脚本 |

---

## 🚀 快速开始

### 方式一：一键启动（推荐）

```
右键 full_spoof.bat -> 以管理员身份运行 -> 选择 [1]
```

### 方式二：命令行

```cmd
# 一键全系统伪装
HWIDChanger.exe -r --full-usermode

# 恢复原始值
HWIDChanger.exe --restore-registry
HWIDChanger.exe --disable-appinit
```

---

## 📋 命令参考

### 基础命令

| 命令 | 说明 |
|------|------|
| `-r, --random` | 生成随机硬件ID |
| `-s, --save <文件>` | 保存配置到文件 |
| `-c, --config <文件>` | 加载配置文件 |
| `-h, --help` | 显示帮助 |

### 伪装命令

| 命令 | 说明 |
|------|------|
| `--full-usermode` | **全系统用户态伪装（推荐）** |
| `--registry-global` | 仅修改注册表 |
| `--enable-appinit` | 启用AppInit DLL注入 |
| `-n, --name <程序>` | 注入到指定程序 |
| `-p, --pid <PID>` | 注入到指定进程ID |

### 恢复命令

| 命令 | 说明 |
|------|------|
| `--restore-registry` | 恢复注册表原始值 |
| `--disable-appinit` | 禁用AppInit注入 |

---

## 🔧 伪装模式详解

### 模式1：全系统用户态伪装 `--full-usermode`（推荐）

```cmd
HWIDChanger.exe -r --full-usermode
```

**效果：**
- ✅ 修改注册表（所有程序生效）
- ✅ 启用AppInit自动注入（新启动的GUI程序自动Hook）

**伪装内容：**
- MachineGuid（机器唯一标识）
- ProductId（Windows产品ID）
- ComputerName（计算机名）
- HwProfileGuid（硬件配置文件）
- MAC地址（注册表级别）

**恢复：**
```cmd
HWIDChanger.exe --restore-registry
HWIDChanger.exe --disable-appinit
```

---

### 模式2：仅注册表修改 `--registry-global`

```cmd
HWIDChanger.exe -r -s config.cfg --registry-global
```

**效果：**
- ✅ 所有程序读取注册表都是假值
- ❌ WMI查询仍返回真值（需要进程注入）

---

### 模式3：进程注入 `-n 程序名`

```cmd
HWIDChanger.exe -c config.cfg -n game.exe
```

**效果：**
- ✅ 目标程序所有API返回假值
- ❌ 其他程序不受影响

**伪装的API：**
- `GetVolumeInformation` - 卷序列号
- `DeviceIoControl` - 磁盘序列号
- `GetAdaptersInfo` - MAC地址
- `GetSystemFirmwareTable` - SMBIOS数据
- `RegQueryValueEx` - 注册表值

---

## 📝 配置文件格式

```ini
# HWID Changer Configuration

# 标志位
flags = 0xFFFF

# 磁盘
disk_serial = "WD-WMC4N0123456"
disk_model = "Virtual HD ATA Device"
volume_serial = 0x12345678

# BIOS
bios_serial = "ABC123456789"
bios_vendor = "American Megatrends Inc."

# 主板
board_serial = "XYZ987654321"
board_product = "B460M-PLUS"

# 系统
system_uuid = "12345678-1234-1234-1234-123456789ABC"
machine_guid = "87654321-4321-4321-4321-CBA987654321"
product_id = "00000-000-0000000-00000"
computer_name = "MY-PC"

# 网络
mac_address = "00:11:22:33:44:55"

# CPU
cpu_id = "1234567890ABCDEF"
```

---

## ✅ 验证伪装效果

### 方法1：使用 HWIDTest.exe

```cmd
HWIDTest.exe
```

### 方法2：使用系统命令

```cmd
reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid
```

### 方法3：使用 PowerShell

```powershell
(Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid
```

---

## ⚠️ 注意事项

1. **需要管理员权限**
   - 右键"以管理员身份运行"

2. **备份自动保存**
   - 首次修改时自动备份到 `hwid_backup.dat`
   - 恢复命令会读取此备份

3. **部分修改需要重启**
   - ComputerName 修改需要重启生效
   - MAC地址可能需要禁用/启用网卡

4. **AppInit注入限制**
   - 仅影响加载 user32.dll 的GUI程序
   - Windows 8+ 可能需要禁用安全启动

5. **WMI查询**
   - 注册表修改不影响WMI查询
   - WMI返回硬件直接值
   - 需要进程注入才能伪装WMI

---

## 🔬 测试验证

运行自动化测试：

```cmd
HWIDSelfTest.exe
```

预期结果：`26 tests passed, 0 failed`

---

## 📊 测试通过的功能

| 功能 | 状态 |
|------|------|
| 配置生成 | ✅ 通过 |
| 配置保存/加载 | ✅ 通过 |
| 卷序列号Hook | ✅ 通过 |
| 磁盘序列号Hook | ✅ 通过 |
| MAC地址Hook | ✅ 通过 |
| 注册表Hook | ✅ 通过 |
| 多Hook同时工作 | ✅ 通过 |
| 注册表永久修改 | ✅ 通过 |
| 注册表恢复 | ✅ 通过 |
| AppInit注入 | ✅ 通过 |
| 进程注入 | ✅ 通过 |

---

## 📁 目录结构

```
HWIDChanger/
├── HWIDChanger.exe      # 主程序
├── HWIDChanger.dll      # Hook DLL
├── HWIDTest.exe         # 硬件信息工具
├── HWIDSelfTest.exe     # 自测试工具
├── full_spoof.bat       # 一键脚本
├── README.md            # 本文档
├── hwid_backup.dat      # 备份文件（自动生成）
└── *.cfg                # 配置文件（自动生成）
```

---

## 版本信息

- **版本**: 2.0
- **架构**: x64
- **编译器**: MinGW-w64 GCC
- **依赖**: 无外部依赖，纯静态编译

---

## 免责声明

本工具仅供学习和测试使用，请勿用于非法用途。使用本工具造成的任何后果由使用者自行承担。

