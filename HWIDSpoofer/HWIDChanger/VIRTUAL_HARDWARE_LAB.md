# Guest-owned BIOS, baseboard and disk serials (QEMU lab)

This lab configures **virtual hardware before Windows boots**. It is not a
host machine HWID modifier and does not make a third-party process on the
host report different hardware. The host firmware, motherboard and disks
remain unchanged.

QEMU supports serials in SMBIOS type 1 (system information) and type 2
(baseboard), and a serial property on the virtual IDE disk device.
References:
- https://www.qemu.org/docs/master/system/invocation.html
- https://github.com/qemu/qemu/blob/master/docs/qdev-device-use.txt

## Requirements

- QEMU with qemu-system-x86_64 available on PATH.
- A bootable Windows guest image (qcow2, raw or img) prepared in advance.
- PowerShell 5.1 or newer on the VM host.
- Sufficient space and memory for a guest VM.

QEMU is not included in this repository. A Windows Sandbox is not itself a
QEMU guest; running an additional emulated guest from Sandbox may be slow.

## Preview the profile without launching anything

    powershell -ExecutionPolicy Bypass -File .\tools\VirtualHardwareProfile.ps1 -Image C:\VirtualLab\guest.qcow2 -SystemId LAB-SYSTEM-001 -BoardId LAB-BOARD-001 -DriveId LAB-DISK-001

## Boot the guest

    powershell -ExecutionPolicy Bypass -File .\tools\VirtualHardwareProfile.ps1 -Image C:\VirtualLab\guest.qcow2 -SystemId LAB-SYSTEM-001 -BoardId LAB-BOARD-001 -DriveId LAB-DISK-001 -Run

Only the guest VM is affected. Networking is disabled, and QEMU runs in
snapshot mode: disk writes are discarded when the VM stops. The OS image
needs to be compatible with a legacy IDE virtual disk. A Windows installation
that relies on a different disk controller may not boot with this lab
configuration. This script does not install Windows or convert disk images.

## Verify inside the guest Windows installation

After booting, copy `tools/Verify-VirtualHardwareGuest.ps1` into the guest
and run:

    powershell -ExecutionPolicy Bypass -File .\Verify-VirtualHardwareGuest.ps1 -SystemId LAB-SYSTEM-001 -BoardId LAB-BOARD-001 -DriveId LAB-DISK-001

The check reports individual PASS/MISMATCH/NOT VERIFIED results through
Windows CIM. It also prints Win32_BIOS.SerialNumber as an additional
diagnostic, because BIOS/system reporting varies by guest firmware and
Windows provider. If Windows does not expose a virtual disk serial, that
field is NOT VERIFIED; no fake success is reported.

This isolated VM solution does NOT complete HWIDChanger.exe's unfinished
user-mode hooking engine. Firmware changes on physical computers and
Windows-wide interception of another process are outside this lab scope.
