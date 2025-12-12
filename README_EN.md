# HWID Spoofer - Hardware ID Spoofing Toolkit

<p align="center">
  <strong>A comprehensive hardware identifier spoofing solution for development and testing purposes</strong>
</p>

<p align="center">
  <a href="#features">Features</a> •
  <a href="#supported-hardware-ids">Supported IDs</a> •
  <a href="#operation-modes">Modes</a> •
  <a href="#quick-start">Quick Start</a> •
  <a href="#building">Building</a> •
  <a href="#documentation">Documentation</a>
</p>

---

## Overview

HWID Spoofer is a powerful Windows-based toolkit designed for spoofing hardware identifiers and machine fingerprints. It provides both **user-mode DLL injection** and **kernel-mode driver** approaches, offering flexibility for various testing scenarios.

Whether you're developing software licensing systems, testing hardware detection mechanisms, or researching system fingerprinting techniques, this toolkit provides comprehensive coverage of Windows hardware identification APIs.

## Features

| Feature | Description |
|---------|-------------|
| 🔄 **Dual Operation Mode** | User-mode hooks for easy deployment, kernel driver for deeper coverage |
| 🎯 **Comprehensive Coverage** | Intercepts 15+ hardware identification methods |
| ⚡ **Lightweight** | Single executable (~50KB) with no external dependencies |
| 🔧 **Flexible Configuration** | JSON/CFG config files with random generation support |
| 💾 **Backup & Restore** | Automatic backup of original values with one-click restore |
| 🛡️ **WMI Interception** | Full WMI query spoofing support |
| 📡 **Network Spoofing** | MAC address modification at multiple API levels |
| 🖥️ **SMBIOS Support** | Complete firmware table manipulation |

## Supported Hardware IDs

| Hardware Type | User-Mode | Kernel Driver | API/Method |
|---------------|:---------:|:-------------:|------------|
| **Disk Serial Number** | ✅ | ✅ | DeviceIoControl, STORAGE_QUERY_PROPERTY |
| **Volume Serial** | ✅ | ✅ | GetVolumeInformation |
| **CPU ID** | ✅ | ⚠️ | WMI: Win32_Processor |
| **BIOS Serial** | ✅ | ✅ | WMI: Win32_BIOS, SMBIOS |
| **Baseboard Serial** | ✅ | ✅ | WMI: Win32_BaseBoard, SMBIOS |
| **System UUID** | ✅ | ✅ | WMI: Win32_ComputerSystemProduct |
| **MachineGuid** | ✅ | ❌ | Windows Registry |
| **MAC Address** | ✅ | ⚠️ | GetAdaptersInfo/Addresses |
| **SMBIOS Data** | ✅ | ✅ | GetSystemFirmwareTable |
| **Product ID** | ✅ | ❌ | Windows Registry |
| **Computer Name** | ✅ | ❌ | Windows Registry |
| **Hardware Profile GUID** | ✅ | ❌ | Windows Registry |

> ⚠️ = Partial support, may require additional configuration

## Operation Modes

### 1. User-Mode (Recommended for Testing)

The user-mode approach uses DLL injection with API hooking via [MinHook](https://github.com/TsudaKageyu/minhook):

- **No driver installation required**
- **No administrator privileges for basic use**
- **Simple one-click injection**
- **Covers 80-90% of common detection scenarios**
- **Ideal for quick testing and development**

#### Hooked APIs

| API | Module | Purpose |
|-----|--------|---------|
| `RegQueryValueExW/A` | advapi32.dll | Registry value queries |
| `RegGetValueW/A` | advapi32.dll | Registry value queries |
| `DeviceIoControl` | kernel32.dll | Device I/O control |
| `GetVolumeInformationW/A` | kernel32.dll | Volume information |
| `GetAdaptersInfo` | iphlpapi.dll | Network adapter info |
| `GetAdaptersAddresses` | iphlpapi.dll | Network addresses |
| `GetSystemFirmwareTable` | kernel32.dll | Firmware tables |
| `SetupDiGetDeviceRegistryPropertyW` | setupapi.dll | Device properties |
| `IWbemClassObject::Get` | wbemuuid.lib | WMI queries |

### 2. Kernel Driver Mode (Advanced)

For scenarios requiring deeper system integration:

- **Intercepts at the lowest level**
- **More comprehensive coverage**
- **Requires test signing mode or valid code signature**
- **Ideal for complete hardware simulation**

## Quick Start

### One-Click System Spoofing

```bash
# Generate random IDs and apply full system spoofing
HWIDChanger.exe -r --full-usermode

# View current hardware information
HWIDTest.exe

# Restore original values
HWIDChanger.exe --restore-registry
HWIDChanger.exe --disable-appinit
```

### Process-Specific Injection

```bash
# Inject into a specific process by name
HWIDChanger.exe -r -n targetapp.exe

# Inject into a specific process by PID
HWIDChanger.exe -r -p 1234

# Wait for process to start, then inject
HWIDInjector.exe -w targetapp.exe
```

### Using Custom Configuration

```bash
# Load custom config and apply
HWIDChanger.exe -c my_config.cfg --full-usermode

# Generate random IDs and save to file
HWIDChanger.exe -r -s my_config.cfg
```

## Command Line Reference

| Parameter | Description |
|-----------|-------------|
| `-r, --random` | Generate random hardware IDs |
| `-a, --apply` | Apply spoofing |
| `-d, --driver` | Load kernel driver |
| `-p, --pid <PID>` | Inject into process by PID |
| `-n, --name <name>` | Inject into process by name |
| `-c, --config <file>` | Load configuration file |
| `-s, --save <file>` | Save configuration to file |
| `--full-usermode` | Full system user-mode spoofing |
| `--registry-global` | Registry modification only |
| `--enable-appinit` | Enable AppInit DLL injection |
| `--restore-registry` | Restore original registry values |
| `--disable-appinit` | Disable AppInit injection |
| `-h, --help` | Display help information |

## Configuration

### JSON Format (hwid_config.json)

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

### CFG Format (config.cfg)

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

> **Note:** Leave any value empty to auto-generate a random value.

## Building from Source

### Prerequisites

- Windows 10/11
- Visual Studio 2019 or later
- CMake 3.16+
- MinHook library (via vcpkg or manual installation)
- Windows Driver Kit (WDK) - for kernel driver only

### Install Dependencies

```bash
# Using vcpkg (recommended)
vcpkg install minhook:x64-windows
vcpkg install minhook:x86-windows

# Or manually download from:
# https://github.com/TsudaKageyu/minhook
# Place files in ThirdParty/minhook/
```

### Build User-Mode Components

```bash
cd HWIDSpoofer
mkdir build && cd build

# 64-bit build
cmake .. -A x64
cmake --build . --config Release

# 32-bit build
cmake .. -A Win32
cmake --build . --config Release
```

### Build Kernel Driver (Optional)

```bash
# Using Developer Command Prompt for VS
cd SpooferDriver
msbuild HWIDSpooferDriver.vcxproj /p:Configuration=Release /p:Platform=x64
```

### Quick Build Script

```bash
# Use the provided build script
build.bat
```

## Project Structure

```
HWIDSpoofer/
├── Common/                     # Shared headers and type definitions
│   ├── hwid_types.h           # Hardware ID type definitions
│   ├── config.h               # Configuration manager
│   └── shared.h               # User/kernel shared structures
│
├── HWIDChanger/               # Main executable
│   ├── main.c                 # Entry point
│   ├── hwid_core.c            # Core functionality
│   ├── hwid_hooks.c           # Hook implementations
│   ├── hwid_dll.c             # DLL module
│   ├── registry_spoof.c       # Registry spoofing
│   └── driver/                # Kernel driver source
│
├── HWIDChangerGUI/            # GUI Application (C#)
│   ├── Program.cs
│   └── HWIDService.cs
│
├── SpooferDLL/                # User-mode Hook DLL
│   ├── include/
│   │   ├── hook_manager.h     # Hook management
│   │   ├── hooks.h            # Hook declarations
│   │   └── logger.h           # Logging system
│   └── src/
│       ├── dllmain.cpp        # DLL entry point
│       ├── hook_manager.cpp   # Hook management impl
│       └── hooks/
│           ├── registry_hook.cpp   # Registry hooks
│           ├── disk_hook.cpp       # Disk hooks
│           ├── network_hook.cpp    # Network hooks
│           ├── wmi_hook.cpp        # WMI hooks
│           ├── firmware_hook.cpp   # Firmware hooks
│           └── setupapi_hook.cpp   # SetupAPI hooks
│
├── SpooferDriver/             # Kernel Driver (Optional)
│   ├── driver.c               # Driver main
│   ├── disk_spoof.c           # Disk spoofing
│   ├── smbios_spoof.c         # SMBIOS spoofing
│   └── irp_hook.c             # IRP hooking
│
├── Injector/                  # DLL Injector
│   └── src/
│       ├── main.cpp           # Injector main
│       ├── injector.cpp       # Injection logic
│       └── process_utils.cpp  # Process utilities
│
├── ThirdParty/                # Third-party libraries
│   └── minhook/               # MinHook library
│
├── Config/
│   └── hwid_config.json       # Sample configuration
│
├── CMakeLists.txt
└── README.md
```

## API Reference

### Exported Functions (SpooferDLL.dll)

```cpp
// Enable hardware spoofing
BOOL EnableSpoofer();

// Disable hardware spoofing
BOOL DisableSpoofer();

// Reload configuration from file
BOOL ReloadConfig();

// Generate random hardware IDs
BOOL GenerateRandomConfig();

// Get count of active hooks
DWORD GetActiveHookCount();

// Check if specific hook is enabled
BOOL IsHookEnabled(const char* hook_name);
```

## Kernel Driver Usage

### Enable Test Signing Mode

```bash
# Run as Administrator
bcdedit /set testsigning on
# Reboot required
```

### Install Driver

```bash
cd driver
install.bat

# Or manually:
sc create HWIDSpoofer type=kernel binPath=C:\path\to\HWIDSpoofer.sys
sc start HWIDSpoofer
```

### Uninstall Driver

```bash
sc stop HWIDSpoofer
sc delete HWIDSpoofer
```

### Use with Driver Mode

```bash
HWIDChanger.exe -d
```

## Troubleshooting

### Q: Why can some applications still detect real hardware IDs?

**A:** They may be using:
- Direct driver communication
- Unhooked APIs or methods
- Kernel-level detection

Consider using kernel driver mode for more comprehensive coverage.

### Q: Injection fails, what should I check?

**A:** Verify:
1. Running with Administrator privileges
2. Architecture match (32-bit DLL for 32-bit process, 64-bit for 64-bit)
3. Antivirus is not blocking the injection
4. Target process is not protected

### Q: How to use different IDs for each session?

**A:** Either:
- Delete the configuration file (auto-generates random values)
- Use the `-r` flag each time
- Call `GenerateRandomConfig()` programmatically

### Q: Changes don't seem to take effect?

**A:** Some changes require:
- System reboot (Computer Name)
- Network adapter restart (MAC address)
- Application restart (cached values)

## Important Notes

⚠️ **This tool is intended for development and testing purposes only.**

- **Do not** use for bypassing software licensing or anti-cheat systems
- Administrator privileges required for full functionality
- Kernel driver requires test signing mode or valid code signature
- Recommended to test in a virtual machine first
- Some antivirus software may flag this tool as suspicious
- Always backup your system before making registry modifications

## License

MIT License

## Changelog

### v2.0
- Added full system user-mode spoofing
- AppInit DLL injection support
- Automatic backup and restore functionality
- Improved registry spoofing
- Enhanced CLI interface

### v1.0
- Initial release
- User-mode API hooking support
- Basic kernel driver framework
- Configuration file support
- DLL injector tool

---

<p align="center">
  <sub>Built for developers, by developers. Use responsibly.</sub>
</p>

