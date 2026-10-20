# Build and recovery notes

## Two independent build trees

This repository contains two separate applications:

| Path | Output | Toolchain |
| --- | --- | --- |
| HWIDSpoofer/HWIDChanger/ | v2.0 HWIDChanger.exe + HWIDChanger.dll + HWIDTest.exe | Windows + MinGW-w64 GCC |
| HWIDSpoofer/ | Legacy HWIDInjector and SpooferDLL prototype | Windows + Visual Studio + CMake + MinHook |

The v2.0 CMake project declares version 2.0.0; the legacy root project is independent. The main README incorrectly conflates the two build systems.

## Build v2 on Windows

Install MinGW-w64 GCC and put gcc in PATH.

    cd HWIDSpoofer\HWIDChanger
    build.bat

Or with MinGW CMake and Ninja:

    cmake -S HWIDSpoofer\HWIDChanger -B build-v2 -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build-v2

The v2 hook code currently uses GCC-only statement expressions and cannot be compiled with MSVC. CMake now reports this explicitly.

## Legacy build

HWIDSpoofer/build.bat requires Visual Studio, CMake and MinHook. This repository only includes a MinHook header, not its implementation. Supply MinHook from an installed package (such as vcpkg), or vendor official source. The build now fails fast instead of silently claiming success.

## Safety changes on this branch

- Make and validate a durable hwid_backup.dat before permanent registry modifications. A corrupt/incompatible backup causes the operation to fail without writes.
- Preserve the legacy registry-backup layout for users who already have backups. Do not publish these files.
- Skip registry-global HwProfileGuid, ActiveComputerName and network adapter NetworkAddress writes until full restoration support exists for them. Per-process hooks are separate.
- Persist AppInit original values in hwid_appinit_backup.dat and refuse destructive clearing if no valid backup exists.
- Refuse replacing a pre-existing, unrelated AppInit DLL list and do not weaken RequireSignedAppInit_DLLs policy.
- Report incomplete registry writes and attempt rollback, but Windows registry updates are not atomic transactions.
- Never delete your only known-good backup, especially if an earlier release changed your system.

## Remaining release blockers

1. Actually build and exercise x64 v2 on Windows 10/11 VMs. CI compiles without running system-changing executables.
2. Add fault-injection tests for partial registry write and recovery failures.
3. Replace home-grown API prologue patching with a reviewed thread-safe library.
4. Audit driver request handling and unloading (kernel stability unverified).
5. Resolve mismatched documentation, stale binary artifacts, and legacy project source dependencies.
6. Protect local backups with restrictive file permissions and store them outside the source checkout.
7. Add integration tests confirming consistent hardware values across multiple supported APIs.

Previously published backups may remain in the repository's Git history even after removal from the current branch.
