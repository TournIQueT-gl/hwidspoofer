# HWID Spoofer - 硬件信息伪装工具

一个用于开发测试的硬件序列号和机器码伪装工具，支持**用户态DLL注入**和**内核驱动**两种模式。

## 功能特性

### 支持伪装的硬件信息

| 类型 | 用户态 | 内核驱动 | 说明 |
|------|--------|----------|------|
| 磁盘序列号 | ✅ | ✅ | DeviceIoControl, STORAGE_QUERY_PROPERTY |
| 卷序列号 | ✅ | ✅ | GetVolumeInformation |
| CPU ID | ✅ | ⚠️ | WMI: Win32_Processor |
| BIOS序列号 | ✅ | ✅ | WMI: Win32_BIOS, SMBIOS |
| 主板序列号 | ✅ | ✅ | WMI: Win32_BaseBoard, SMBIOS |
| 系统UUID | ✅ | ✅ | WMI: Win32_ComputerSystemProduct |
| MachineGuid | ✅ | ❌ | 注册表 |
| MAC地址 | ✅ | ⚠️ | GetAdaptersInfo/Addresses |
| SMBIOS数据 | ✅ | ✅ | GetSystemFirmwareTable |

> ⚠️ 部分功能需要更深层的Hook或额外处理

### 工作模式

#### 1. 用户态模式（推荐用于测试）
- 通过DLL注入实现API Hook
- 无需管理员权限安装驱动
- 简单易用，一键注入
- 覆盖80-90%的常见场景

#### 2. 内核驱动模式（更彻底）
- 更底层的拦截
- 需要测试模式或代码签名
- 适合需要完全模拟的场景

## 项目结构

```
HWIDSpoofer/
├── Common/                     # 公共头文件和类型定义
│   ├── hwid_types.h           # 硬件ID类型定义
│   ├── config.h               # 配置管理器
│   └── shared.h               # 用户态/内核态共享结构
│
├── SpooferDLL/                # 用户态Hook DLL
│   ├── include/
│   │   ├── hook_manager.h     # Hook管理器
│   │   ├── hooks.h            # Hook声明
│   │   └── logger.h           # 日志系统
│   └── src/
│       ├── dllmain.cpp        # DLL入口
│       ├── hook_manager.cpp   # Hook管理实现
│       ├── hooks/
│       │   ├── registry_hook.cpp   # 注册表Hook
│       │   ├── disk_hook.cpp       # 磁盘Hook
│       │   ├── network_hook.cpp    # 网络Hook
│       │   ├── wmi_hook.cpp        # WMI Hook
│       │   ├── firmware_hook.cpp   # 固件Hook
│       │   └── setupapi_hook.cpp   # SetupAPI Hook
│       └── utils/
│           ├── config_loader.cpp   # 配置加载
│           └── logger.cpp          # 日志实现
│
├── SpooferDriver/             # 内核驱动（可选）
│   ├── driver.c               # 驱动主体
│   ├── disk_spoof.c           # 磁盘伪装
│   ├── smbios_spoof.c         # SMBIOS伪装
│   └── sources                # WDK构建文件
│
├── Injector/                  # DLL注入器
│   ├── include/
│   │   └── injector.h
│   └── src/
│       ├── main.cpp           # 主程序
│       └── injector.cpp       # 注入实现
│
├── ThirdParty/                # 第三方库
│   └── minhook/               # MinHook库
│
├── Config/
│   └── hwid_config.json       # 配置文件
│
├── CMakeLists.txt
└── README.md
```

## 编译说明

### 前置要求

- Windows 10/11
- Visual Studio 2019 或更高版本
- CMake 3.16+
- MinHook库（通过vcpkg安装或手动下载）

### 安装MinHook

```bash
# 方法1: 使用vcpkg
vcpkg install minhook:x64-windows
vcpkg install minhook:x86-windows

# 方法2: 手动下载
# 从 https://github.com/TsudaKageyu/minhook 下载
# 将文件放入 ThirdParty/minhook/ 目录
```

### 编译用户态组件

```bash
cd HWIDSpoofer
mkdir build
cd build

# 64位
cmake .. -A x64
cmake --build . --config Release

# 32位
cmake .. -A Win32
cmake --build . --config Release
```

### 编译内核驱动（可选）

需要安装Windows Driver Kit (WDK)：
- [下载WDK](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)

```bash
# 使用Developer Command Prompt
cd SpooferDriver
msbuild HWIDSpooferDriver.vcxproj /p:Configuration=Release /p:Platform=x64
```

## 使用方法

### 快速开始（用户态）

1. **编译项目**或下载预编译版本

2. **编辑配置文件** `hwid_config.json`
   - 修改想要伪装的硬件信息
   - 或留空使用随机生成的值

3. **运行注入器**
   ```bash
   # 注入到指定进程
   HWIDInjector.exe -n 目标程序.exe
   
   # 或指定PID
   HWIDInjector.exe -p 1234
   
   # 等待进程启动后注入
   HWIDInjector.exe -w 目标程序.exe
   ```

### 命令行参数

```
HWIDInjector.exe [选项]

选项:
  -p, --pid <PID>      注入到指定PID的进程
  -n, --name <名称>    注入到指定名称的进程
  -w, --wait <名称>    等待进程启动后注入
  -d, --dll <路径>     指定DLL路径（默认: SpooferDLL.dll）
  -e, --eject          卸载DLL（而非注入）
  -l, --list           列出所有运行中的进程
  -c, --check          检查DLL是否已注入
  -h, --help           显示帮助信息

示例:
  HWIDInjector.exe -n notepad.exe
  HWIDInjector.exe -p 1234 -d C:\path\to\SpooferDLL.dll
  HWIDInjector.exe -w game.exe
  HWIDInjector.exe -l
```

### 配置文件说明

```json
{
  // 伪装开关（true启用，false禁用）
  "spoof_disk": true,        // 磁盘序列号
  "spoof_cpu": true,         // CPU ID
  "spoof_baseboard": true,   // 主板信息
  "spoof_bios": true,        // BIOS信息
  "spoof_system": true,      // 系统信息
  "spoof_network": true,     // MAC地址
  "spoof_smbios": true,      // SMBIOS数据
  "spoof_registry": true,    // 注册表
  "spoof_wmi": true,         // WMI查询

  // 硬件信息（留空则自动生成随机值）
  "disk_serial": "WD-WMC4N0123456",
  "mac_address": "00:1A:2B:3C:4D:5E",
  "machine_guid": "87654321-4321-1234-DCBA-CBA987654321",
  // ... 其他配置项
}
```

### 使用内核驱动（高级）

1. **启用测试模式**
   ```bash
   # 以管理员身份运行
   bcdedit /set testsigning on
   # 重启电脑
   ```

2. **安装驱动**
   ```bash
   # 以管理员身份运行
   sc create HWIDSpoofer type=kernel binPath=C:\path\to\HWIDSpoofer.sys
   sc start HWIDSpoofer
   ```

3. **卸载驱动**
   ```bash
   sc stop HWIDSpoofer
   sc delete HWIDSpoofer
   ```

## API参考

### 导出函数（SpooferDLL.dll）

```cpp
// 启用伪装
BOOL EnableSpoofer();

// 禁用伪装
BOOL DisableSpoofer();

// 重新加载配置
BOOL ReloadConfig();

// 生成随机配置
BOOL GenerateRandomConfig();

// 获取活动Hook数量
DWORD GetActiveHookCount();

// 检查指定Hook是否启用
BOOL IsHookEnabled(const char* hook_name);
```

## 技术细节

### Hook的API列表

| API | 模块 | 用途 |
|-----|------|------|
| RegQueryValueExW/A | advapi32.dll | 注册表值读取 |
| RegGetValueW/A | advapi32.dll | 注册表值读取 |
| DeviceIoControl | kernel32.dll | 设备IO控制 |
| GetVolumeInformationW/A | kernel32.dll | 卷信息 |
| GetAdaptersInfo | iphlpapi.dll | 网卡信息 |
| GetAdaptersAddresses | iphlpapi.dll | 网卡地址 |
| GetSystemFirmwareTable | kernel32.dll | 固件表 |
| SetupDiGetDeviceRegistryPropertyW | setupapi.dll | 设备属性 |
| IWbemClassObject::Get | wbemuuid.lib | WMI查询 |

### 常见问题

**Q: 为什么某些程序仍能获取真实硬件信息？**
- A: 可能使用了驱动直接读取或其他未被Hook的方法。考虑使用内核驱动模式。

**Q: 注入失败怎么办？**
- A: 检查：1) 管理员权限 2) 32/64位匹配 3) 杀毒软件

**Q: 如何对每次运行使用不同的硬件ID？**
- A: 删除配置文件或使用`GenerateRandomConfig()`函数

## 注意事项

⚠️ **本工具仅供开发测试使用**

- 请勿用于规避软件授权或反作弊系统
- 某些场景可能需要管理员权限
- 内核驱动需要测试模式或有效签名
- 建议在虚拟机中测试

## 许可证

MIT License

## 更新日志

### v1.0.0
- 初始版本
- 支持用户态API Hook
- 基础内核驱动框架
- 配置文件支持
- 注入器工具

