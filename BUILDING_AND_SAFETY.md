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


## Sandbox verification matrix (October 2026)

Windows Sandbox verified the registry changes and recovery:

| Readback surface | Current result | Scope |
| --- | --- | --- |
| MachineGuid, ProductId, ComputerName registry key | CHANGED and restored exactly | Registry values, not physical firmware |
| MAC from Get-NetAdapter | CHANGED and restored exactly | Standalone, driver-supported Sandbox adapter test; NOT integrated into the EXE |
| BIOS/board via Win32_BIOS / Win32_BaseBoard | UNCHANGED | Hypervisor-provided SMBIOS; no firmware modifications implemented |
| Disk serial via Win32_DiskDrive | NOT VERIFIED | Sandbox returned an empty disk serial |
| Disk volume serial | Not verified | Different identifier from a physical disk serial |
| User-mode DLL hooks in external processes | NOT VERIFIED | Legacy hooking is unsafe to treat as production-ready |
| Kernel global mode | UNAVAILABLE | No sys file built in the v2 build; protocol differs between caller and driver |

**Do not enable Windows test signing or run the experimental driver to address missing identifiers.**
The driver and user-mode client disagree on device names and IOCTL definitions and have not been reviewed for kernel stability.

### Read-only before/after measurements

Use the tools/Verify-HWID.ps1 script in Windows Sandbox (no administrator access required for most readings):

1. powershell -ExecutionPolicy Bypass -File .\tools\Verify-HWID.ps1 -Phase Before
2. Perform one selected and independently reversible test.
3. powershell -ExecutionPolicy Bypass -File .\tools\Verify-HWID.ps1 -Phase After

Results are under Desktop\HWID-Verification: Before.json, After.json and Comparison.csv.
If Windows or virtualized APIs do not expose a value, the result is **NOT VERIFIED**, never a false pass.
ComputerNameKey and live ActiveName intentionally remain separate because name changes can require a reboot.

### Engineering order for reliable hardware reporting

1. Obtain independent process-level measurements for the specific APIs the product claims to intercept.
2. Replace the original in-memory prologue patching with a reviewed, thread-safe hooking implementation before enabling hooks.
3. Validate SMBIOS and storage descriptors with strict offset and length checks; refuse malformed or absent serials.
4. Keep MAC overrides explicitly opt-in, restricted to driver-supported adapters, with rollback on interruption or verification failure.
5. Provide a single before/after/restore report and show unsupported identifiers honestly. Changing an OS/driver report is not changing physical firmware.

**This branch is a draft, not a release.**


## Integrated Sandbox MAC experiment (new, NOT yet validated in a new build)

The main v2 console now offers [8] MAC override/restore and the following CLI actions:

- `--mac-test`: apply an adapter-supported random local-unicast MAC, verify it, and immediately restore it.
- `--mac-apply`: apply a random MAC to the first active adapter and persist the original before the attempt.
- `--mac-status`: show saved original and current adapter MAC.
- `--mac-restore`: restore the original address and verify readback.
- `--show-hardware`: invoke the read-only Windows hardware information report (also menu [5]).

These features run bundled PowerShell companions, not a kernel driver. Both build.bat and CMake copy these companions into the executable directory. MAC write actions are restricted to the disposable Windows Sandbox user and may temporarily disconnect the adapter. The durable adapter backup is at LOCALAPPDATA/HWIDChanger/mac-original.json. An existing backup blocks subsequent apply attempts. If recovery fails, discard that Sandbox session. Never delete the backup manually to work around a recovery failure.

The old menu [3] and CLI global driver mode are explicitly unavailable on the draft branch. The legacy in-process hooks and process injection are marked experimental and have not been release-qualified.

**Run a clean Windows Sandbox integration test:**

With MSYS2 UCRT64 GCC present, run tools/SmokeTest-Sandbox.ps1. It downloads the current branch, builds it, checks menu [5] via --show-hardware, and performs --mac-test with independent before/after readback. The log is written to Desktop/HWID-App-Check/smoke-result.txt.

No success is claimed for BIOS firmware, motherboard firmware, physical disk serial or third-party API interception. The main program's generated config does not demonstrate those hardware IDs changed.
