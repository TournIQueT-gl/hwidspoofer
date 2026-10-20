# Reliability audit (in progress)

This branch contains non-invasive build and documentation fixes. It has **not** been tested on Windows and does **not** certify the driver, injector, or spoofing functionality.

## Release provenance

The `HWIDChanger_v2.0_Release` binaries and the `HWIDSpoofer/CMakeLists.txt` build outputs use different names. The latter currently builds `SpooferDLL` and `HWIDInjector` and declares version 1.0.0. Do not assume those source targets reproduce the v2.0 binaries without tracing their provenance.

## Safety checks before using system-modifying features

- Test in a disposable Windows VM with a snapshot.
- Do not distribute machine-specific `hwid_backup.dat` files. Backups should be generated per machine and protected from unauthorized access.
- Validate backup completeness and restore behavior before enabling registry changes.
- Confirm rollback after interruption, missing files, insufficient privileges, and restart.
- Do not enable global AppInit injection by default.
- Validate kernel-driver loading/unloading and failure paths in a VM with crash dumps enabled.

## Build validation

- Configure and compile x64 and Win32 separately on Windows with CMake, Visual Studio, and dependencies installed.
- Confirm both output directories contain the generated `hwid_config.json`.
- Verify clean builds and compare executable hashes to documented releases.
- Add automated build and smoke tests before release.
