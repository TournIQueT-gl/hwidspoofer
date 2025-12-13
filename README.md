# HWID Spoofer - 硬件信息伪装工具

<p align="center">
  <strong>一个用于开发测试的全面硬件标识伪装解决方案</strong>
</p>

<p align="center">
  <a href="#功能特性">功能特性</a> •
  <a href="#支持伪装的硬件信息">支持的硬件</a> •
  <a href="#工作模式">工作模式</a> •
  <a href="#快速开始">快速开始</a> •
  <a href="#编译说明">编译说明</a> •
  <a href="#文档">文档</a>
</p>

---

## 概述

HWID Spoofer 一个强大的 Windows 硬件标识伪装工具包，用于伪装硬件标识符和机器指纹。提供**用户态 DLL 注入**和**内核驱动**两种方式，可灵活应对各种测试场景。
无论是在开发软件授权系统、测试硬件检测机制，还是研究系统指纹识别技术，本工具包都能全面覆盖 Windows 硬件标识 API。

## 功能特性

| 功能 | 说明 |
|------|------|
| 🔄 **双模式运行** | 用户态Hook便于部署，内核驱动覆盖更深 |
| 🎯 **全面覆盖** | 拦截15+种硬件标识方法 |
| ⚡ **轻量级** | 单个可执行文件（约50KB），无外部依赖 |
| 🔧 **灵活配置** | JSON/CFG 配置文件，支持随机生成 |
| 💾 **备份恢复** | 自动备份原始值，一键恢复 |
| 🛡️ **WMI拦截** | 完整的 WMI 查询伪装支持 |
| 📡 **网络伪装** | 多层 API 级别的 MAC 地址修改 |
| 🖥️ **SMBIOS支持** | 完整的固件表操作 |

## 支持伪装的硬件信息

| 硬件类型 | 用户态 | 内核驱动 | API/方法 |
|----------|:------:|:--------:|----------|
| **磁盘序列号** | ✅ | ✅ | DeviceIoControl, STORAGE_QUERY_PROPERTY |
| **卷序列号** | ✅ | ✅ | GetVolumeInformation |
| **CPU ID** | ✅ | ⚠️ | WMI: Win32_Processor |
| **BIOS序列号** | ✅ | ✅ | WMI: Win32_BIOS, SMBIOS |
| **主板序列号** | ✅ | ✅ | WMI: Win32_BaseBoard, SMBIOS |
| **系统UUID** | ✅ | ✅ | WMI: Win32_ComputerSystemProduct |
| **MachineGuid** | ✅ | ❌ | Windows 注册表 |
| **MAC地址** | ✅ | ⚠️ | GetAdaptersInfo/Addresses |
| **SMBIOS数据** | ✅ | ✅ | GetSystemFirmwareTable |
| **ProductID** | ✅ | ❌ | Windows 注册表 |
| **计算机名** | ✅ | ❌ | Windows 注册表 |
| **硬件配置文件GUID** | ✅ | ❌ | Windows 注册表 |

> ⚠️ = 部分支持，可能需要额外配置

## 工作模式

### 1. 用户态模式（推荐用于测试）

用户态方式通过 DLL 注入配合 [MinHook](https://github.com/TsudaKageyu/minhook) 实现 API Hook：

- **无需安装驱动**
- **基本使用无需管理员权限**
- **简单易用，一键注入**
- **覆盖80-90%的常见检测场景**
- **适合快速测试和开发**

#### Hook 的 API 列表

| API | 模块 | 用途 |
|-----|------|------|
| `RegQueryValueExW/A` | advapi32.dll | 注册表值查询 |
| `RegGetValueW/A` | advapi32.dll | 注册表值查询 |
| `DeviceIoControl` | kernel32.dll | 设备IO控制 |
| `GetVolumeInformationW/A` | kernel32.dll | 卷信息 |
| `GetAdaptersInfo` | iphlpapi.dll | 网卡信息 |
| `GetAdaptersAddresses` | iphlpapi.dll | 网卡地址 |
| `GetSystemFirmwareTable` | kernel32.dll | 固件表 |
| `SetupDiGetDeviceRegistryPropertyW` | setupapi.dll | 设备属性 |
| `IWbemClassObject::Get` | wbemuuid.lib | WMI查询 |

### 2. 内核驱动模式（高级）

适用于需要更深层系统集成的场景：

- **最底层拦截**
- **覆盖更全面**
- **需要测试签名模式或有效的代码签名**
- **适合完整的硬件模拟**

## 快速开始

### 一键系统伪装

```bash
# 生成随机ID并应用全系统伪装
HWIDChanger.exe -r --full-usermode

# 查看当前硬件信息
HWIDTest.exe

# 恢复原始值
HWIDChanger.exe --restore-registry
HWIDChanger.exe --disable-appinit
```

### 进程特定注入

```bash
# 按进程名注入到指定进程
HWIDChanger.exe -r -n 目标程序.exe

# 按PID注入到指定进程
HWIDChanger.exe -r -p 1234

# 等待进程启动后注入
HWIDInjector.exe -w 目标程序.exe
```

### 使用自定义配置

```bash
# 加载自定义配置并应用
HWIDChanger.exe -c my_config.cfg --full-usermode

# 生成随机ID并保存到文件
HWIDChanger.exe -r -s my_config.cfg
```

## 命令行参数

| 参数 | 说明 |
|------|------|
| `-r, --random` | 生成随机硬件ID |
| `-a, --apply` | 应用伪装 |
| `-d, --driver` | 加载内核驱动 |
| `-p, --pid <PID>` | 按PID注入进程 |
| `-n, --name <名称>` | 按名称注入进程 |
| `-c, --config <文件>` | 加载配置文件 |
| `-s, --save <文件>` | 保存配置到文件 |
| `--full-usermode` | 全系统用户态伪装 |
| `--registry-global` | 仅注册表修改 |
| `--enable-appinit` | 启用 AppInit DLL 注入 |
| `--restore-registry` | 恢复原始注册表值 |
| `--disable-appinit` | 禁用 AppInit 注入 |
| `-h, --help` | 显示帮助信息 |

## 配置文件说明

### JSON 格式 (hwid_config.json)

```json
{
  "version": "1.0",
  
  "spoof_disk": true,
  "spoof_cpu": true,
  "spoof_baseboard": true,
  "spoof_bios": true,
  "spoof_system": true,
  "spoof_network": true,
  "spoof_smbios": true,
  "spoof_registry": true,
  "spoof_wmi": true,

  "disk_serial": "WD-WMC4N0123456",
  "volume_serial": 305419896,
  
  "processor_id": "BFEBFBFF000906ED",
  "cpu_brand": "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz",
  
  "baseboard_serial": "MB12345678901",
  "baseboard_product": "B460M-PLUS",
  "baseboard_manufacturer": "ASUSTeK COMPUTER INC.",
  
  "bios_serial": "BIOS123456789",
  "bios_manufacturer": "American Megatrends Inc.",
  
  "system_uuid": "12345678-1234-4321-ABCD-123456789ABC",
  "machine_guid": "87654321-4321-1234-DCBA-CBA987654321",
  "product_id": "00330-80000-00000-AA123",
  
  "mac_address": "00:1A:2B:3C:4D:5E"
}
```

### CFG 格式 (config.cfg)

```ini
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

> **提示：** 留空任何值将自动生成随机值。

## 编译说明

### 前置要求

- Windows 10/11
- Visual Studio 2019 或更高版本
- CMake 3.16+
- MinHook库（通过vcpkg安装或手动下载）
- Windows Driver Kit (WDK) - 仅内核驱动需要

### 安装依赖

```bash
# 使用 vcpkg（推荐）
vcpkg install minhook:x64-windows
vcpkg install minhook:x86-windows

# 或手动下载：
# https://github.com/TsudaKageyu/minhook
# 将文件放入 ThirdParty/minhook/ 目录
```

### 编译用户态组件

```bash
cd HWIDSpoofer
mkdir build && cd build

# 64位编译
cmake .. -A x64
cmake --build . --config Release

# 32位编译
cmake .. -A Win32
cmake --build . --config Release
```

### 编译内核驱动（可选）

```bash
# 使用 VS Developer Command Prompt
cd SpooferDriver
msbuild HWIDSpooferDriver.vcxproj /p:Configuration=Release /p:Platform=x64
```

### 快速编译脚本

```bash
# 使用提供的编译脚本
build.bat
```

## 项目结构

```
HWIDSpoofer/
├── Common/                     # 公共头文件和类型定义
│   ├── hwid_types.h           # 硬件ID类型定义
│   ├── config.h               # 配置管理器
│   └── shared.h               # 用户态/内核态共享结构
│
├── HWIDChanger/               # 主程序
│   ├── main.c                 # 入口点
│   ├── hwid_core.c            # 核心功能
│   ├── hwid_hooks.c           # Hook实现
│   ├── hwid_dll.c             # DLL模块
│   ├── registry_spoof.c       # 注册表伪装
│   └── driver/                # 内核驱动源码
│
├── HWIDChangerGUI/            # GUI应用程序 (C#)
│   ├── Program.cs
│   └── HWIDService.cs
│
├── SpooferDLL/                # 用户态Hook DLL
│   ├── include/
│   │   ├── hook_manager.h     # Hook管理
│   │   ├── hooks.h            # Hook声明
│   │   └── logger.h           # 日志系统
│   └── src/
│       ├── dllmain.cpp        # DLL入口
│       ├── hook_manager.cpp   # Hook管理实现
│       └── hooks/
│           ├── registry_hook.cpp   # 注册表Hook
│           ├── disk_hook.cpp       # 磁盘Hook
│           ├── network_hook.cpp    # 网络Hook
│           ├── wmi_hook.cpp        # WMI Hook
│           ├── firmware_hook.cpp   # 固件Hook
│           └── setupapi_hook.cpp   # SetupAPI Hook
│
├── SpooferDriver/             # 内核驱动（可选）
│   ├── driver.c               # 驱动主体
│   ├── disk_spoof.c           # 磁盘伪装
│   ├── smbios_spoof.c         # SMBIOS伪装
│   └── irp_hook.c             # IRP Hook
│
├── Injector/                  # DLL注入器
│   └── src/
│       ├── main.cpp           # 注入器主程序
│       ├── injector.cpp       # 注入逻辑
│       └── process_utils.cpp  # 进程工具
│
├── ThirdParty/                # 第三方库
│   └── minhook/               # MinHook库
│
├── Config/
│   └── hwid_config.json       # 示例配置
│
├── CMakeLists.txt
└── README.md
```

## API参考

### 导出函数（SpooferDLL.dll）

```cpp
// 启用硬件伪装
BOOL EnableSpoofer();

// 禁用硬件伪装
BOOL DisableSpoofer();

// 从文件重新加载配置
BOOL ReloadConfig();

// 生成随机硬件ID
BOOL GenerateRandomConfig();

// 获取活动Hook数量
DWORD GetActiveHookCount();

// 检查指定Hook是否启用
BOOL IsHookEnabled(const char* hook_name);
```

## 内核驱动使用

### 启用测试签名模式

```bash
# 以管理员身份运行
bcdedit /set testsigning on
# 需要重启
```

### 安装驱动

```bash
cd driver
install.bat

# 或手动安装：
sc create HWIDSpoofer type=kernel binPath=C:\path\to\HWIDSpoofer.sys
sc start HWIDSpoofer
```

### 卸载驱动

```bash
sc stop HWIDSpoofer
sc delete HWIDSpoofer
```

### 使用驱动模式

```bash
HWIDChanger.exe -d
```

## 常见问题

### Q: 为什么某些程序仍能获取真实硬件信息？

**A:** 可能使用了：
- 直接驱动通信
- 未被Hook的API或方法
- 内核级检测

考虑使用内核驱动模式以获得更全面的覆盖。

### Q: 注入失败怎么办？

**A:** 请检查：
1. 是否以管理员权限运行
2. 架构是否匹配（32位DLL对应32位进程，64位对应64位）
3. 杀毒软件是否阻止了注入
4. 目标进程是否受保护

### Q: 如何每次使用不同的硬件ID？

**A:** 可以：
- 删除配置文件（自动生成随机值）
- 每次使用 `-r` 参数
- 编程调用 `GenerateRandomConfig()`

### Q: 修改似乎没有生效？

**A:** 某些修改需要：
- 系统重启（计算机名）
- 网卡重启（MAC地址）
- 应用程序重启（缓存的值）

## 注意事项

⚠️ **本工具仅供开发测试使用**

- **请勿**用于规避软件授权或反作弊系统
- 完整功能需要管理员权限
- 内核驱动需要测试签名模式或有效的代码签名
- 建议先在虚拟机中测试
- 某些杀毒软件可能会将本工具标记为可疑
- 修改注册表前请务必备份系统

## 许可证

MIT License

## 更新日志

### v2.0
- 新增全系统用户态伪装
- AppInit DLL注入支持
- 自动备份和恢复功能
- 改进的注册表伪装
- 增强的命令行界面

### v1.0
- 初始版本
- 用户态API Hook支持
- 基础内核驱动框架
- 配置文件支持
- DLL注入器工具

---

<p align="center">
  <sub>为开发者而生。请负责任地使用。</sub>
</p>
