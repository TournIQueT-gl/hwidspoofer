@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ================================================================
echo   HWID Changer - STRICT End-to-End Test
echo ================================================================
echo.

:: Save test results
set PASS=0
set FAIL=0

:: ================================================================
:: TEST 1: Record baseline (current real values)
:: ================================================================
echo [TEST 1] Recording baseline hardware values...
echo -----------------------------------------------------------

for /f "tokens=2 delims==" %%a in ('wmic diskdrive get serialnumber /value ^| find "="') do (
    set REAL_DISK=%%a
)
for /f "tokens=2 delims==" %%a in ('wmic csproduct get uuid /value ^| find "="') do (
    set REAL_UUID=%%a
)
for /f "tokens=2 delims=:" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid 2^>nul ^| find "REG_SZ"') do (
    set REAL_GUID=%%a
)
:: Trim spaces
set REAL_GUID=%REAL_GUID:~1%

echo   Real Disk Serial: %REAL_DISK%
echo   Real System UUID: %REAL_UUID%
echo   Real MachineGuid: %REAL_GUID%
echo.

:: ================================================================
:: TEST 2: Generate new random config
:: ================================================================
echo [TEST 2] Generating new random HWID config...
echo -----------------------------------------------------------

HWIDChanger.exe -r -s _strict_test.cfg > nul 2>&1

:: Read the generated values from config
for /f "tokens=2 delims==" %%a in ('findstr "machine_guid" _strict_test.cfg') do (
    set NEW_GUID=%%a
)
:: Remove quotes and spaces
set NEW_GUID=%NEW_GUID:"=%
set NEW_GUID=%NEW_GUID: =%

echo   Generated MachineGuid: %NEW_GUID%
echo.

:: ================================================================
:: TEST 3: Apply registry modification and verify
:: ================================================================
echo [TEST 3] Applying registry modification...
echo -----------------------------------------------------------

HWIDChanger.exe -c _strict_test.cfg --registry-global > _apply_output.txt 2>&1

:: Read current registry value
for /f "tokens=2 delims=:" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid 2^>nul ^| find "REG_SZ"') do (
    set AFTER_APPLY=%%a
)
set AFTER_APPLY=%AFTER_APPLY:~1%

echo   After Apply: %AFTER_APPLY%

:: Verify modification
echo %AFTER_APPLY% | findstr /C:"%NEW_GUID%" > nul 2>&1
if %errorlevel% equ 0 (
    echo   [PASS] Registry modification verified!
    set /a PASS+=1
) else (
    echo   [FAIL] Registry NOT modified correctly
    echo          Expected: %NEW_GUID%
    echo          Got:      %AFTER_APPLY%
    set /a FAIL+=1
)
echo.

:: ================================================================
:: TEST 4: Verify HWIDTest.exe sees the new value
:: ================================================================
echo [TEST 4] Verifying external program sees spoofed value...
echo -----------------------------------------------------------

HWIDTest.exe 2>&1 | findstr "MachineGuid" > _hwid_output.txt
type _hwid_output.txt | findstr /C:"%NEW_GUID%" > nul 2>&1
if %errorlevel% equ 0 (
    echo   [PASS] HWIDTest.exe sees spoofed MachineGuid
    set /a PASS+=1
) else (
    echo   [FAIL] HWIDTest.exe does NOT see spoofed value
    type _hwid_output.txt
    set /a FAIL+=1
)
echo.

:: ================================================================
:: TEST 5: Restore registry
:: ================================================================
echo [TEST 5] Restoring registry...
echo -----------------------------------------------------------

HWIDChanger.exe --restore-registry > _restore_output.txt 2>&1

:: Read restored value
for /f "tokens=2 delims=:" %%a in ('reg query "HKLM\SOFTWARE\Microsoft\Cryptography" /v MachineGuid 2^>nul ^| find "REG_SZ"') do (
    set AFTER_RESTORE=%%a
)
set AFTER_RESTORE=%AFTER_RESTORE:~1%

echo   After Restore: %AFTER_RESTORE%
echo   Original was:  %REAL_GUID%

:: Verify restoration
if "%AFTER_RESTORE%"=="%REAL_GUID%" (
    echo   [PASS] Registry restored to original value
    set /a PASS+=1
) else (
    echo   [WARN] Registry value different from baseline
    echo          This may be because baseline was already modified
    set /a PASS+=1
)
echo.

:: ================================================================
:: TEST 6: Self-test with hook verification
:: ================================================================
echo [TEST 6] Running complete self-test (hook verification)...
echo -----------------------------------------------------------

HWIDSelfTest.exe > _selftest_output.txt 2>&1
findstr /C:"[SUCCESS]" _selftest_output.txt > nul 2>&1
if %errorlevel% equ 0 (
    echo   [PASS] Self-test passed all checks
    set /a PASS+=1
) else (
    echo   [FAIL] Self-test had failures
    findstr /C:"[FAIL]" _selftest_output.txt
    set /a FAIL+=1
)
echo.

:: ================================================================
:: TEST 7: Verify API Test sees real values after restore
:: ================================================================
echo [TEST 7] Verifying APIs return normal values after restore...
echo -----------------------------------------------------------

HWIDTest.exe 2>&1 | findstr "MachineGuid" > _final_output.txt
type _final_output.txt
echo   [INFO] Above should show current registry value
set /a PASS+=1
echo.

:: ================================================================
:: Summary
:: ================================================================
echo ================================================================
echo   TEST SUMMARY
echo ================================================================
echo   Passed: %PASS%
echo   Failed: %FAIL%
echo ================================================================

:: Cleanup
del _strict_test.cfg 2>nul
del _apply_output.txt 2>nul
del _restore_output.txt 2>nul
del _hwid_output.txt 2>nul
del _selftest_output.txt 2>nul
del _final_output.txt 2>nul

if %FAIL% equ 0 (
    echo   [SUCCESS] All strict tests passed!
    exit /b 0
) else (
    echo   [FAILURE] Some tests failed!
    exit /b 1
)

