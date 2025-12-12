@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

echo.
echo ============================================
echo   HWID Changer - Complete Test Suite
echo ============================================
echo.

set PASS=0
set FAIL=0

:: ============================================
:: Part 1: Basic Function Tests
:: ============================================
echo [Part 1] Basic Function Tests
echo --------------------------------------------

:: Test 1: Help
echo   [1] Help command...
HWIDChanger.exe --help >nul 2>&1
if %errorlevel% equ 0 (
    echo       PASS
    set /a PASS+=1
) else (
    echo       FAIL
    set /a FAIL+=1
)

:: Test 2: Generate Random
echo   [2] Generate random HWID...
HWIDChanger.exe -r -s _test_config.cfg >nul 2>&1
if exist _test_config.cfg (
    echo       PASS
    set /a PASS+=1
) else (
    echo       FAIL
    set /a FAIL+=1
)

:: Test 3: Load Config
echo   [3] Load config file...
HWIDChanger.exe -c _test_config.cfg -q >nul 2>&1
if %errorlevel% equ 0 (
    echo       PASS
    set /a PASS+=1
) else (
    echo       FAIL
    set /a FAIL+=1
)

:: ============================================
:: Part 2: API Verification (Before Spoof)
:: ============================================
echo.
echo [Part 2] API Verification - BEFORE Spoof
echo --------------------------------------------

if not exist HWIDTest.exe (
    echo   [!] HWIDTest.exe not found, skipping API tests
    goto :part3
)

echo   Running HWIDTest.exe to get current hardware info...
echo.
HWIDTest.exe > _before_spoof.txt 2>&1
type _before_spoof.txt
echo.

:part3
:: ============================================
:: Part 3: Registry Modification Test
:: ============================================
echo [Part 3] Registry Modification Test
echo --------------------------------------------

echo   [4] Apply registry modifications...
HWIDChanger.exe -c _test_config.cfg --registry-global > _reg_output.txt 2>&1
findstr /C:"Registry modified" _reg_output.txt >nul 2>&1
if %errorlevel% equ 0 (
    echo       PASS - Registry modified
    set /a PASS+=1
    
    :: Show what was modified
    findstr /C:"[OK]" _reg_output.txt
) else (
    echo       FAIL - Could not modify registry
    set /a FAIL+=1
    type _reg_output.txt
)

:: ============================================
:: Part 4: API Verification (After Registry Mod)
:: ============================================
echo.
echo [Part 4] API Verification - AFTER Registry Modification
echo --------------------------------------------

if exist HWIDTest.exe (
    echo   Running HWIDTest.exe to verify registry changes...
    echo.
    HWIDTest.exe > _after_spoof.txt 2>&1
    
    :: Compare MachineGuid
    echo   Comparing results:
    echo   ---
    findstr /C:"MachineGuid" _before_spoof.txt
    findstr /C:"MachineGuid" _after_spoof.txt
    echo   ---
    findstr /C:"ProductId" _before_spoof.txt
    findstr /C:"ProductId" _after_spoof.txt
    echo   ---
)

:: ============================================
:: Part 5: Restore Registry
:: ============================================
echo.
echo [Part 5] Registry Restore Test
echo --------------------------------------------

echo   [5] Restore registry...
HWIDChanger.exe --restore-registry > _restore_output.txt 2>&1
findstr /C:"Registry restored" _restore_output.txt >nul 2>&1
if %errorlevel% equ 0 (
    echo       PASS - Registry restored
    set /a PASS+=1
) else (
    echo       FAIL - Could not restore registry
    set /a FAIL+=1
    type _restore_output.txt
)

:: ============================================
:: Part 6: Verify Restoration
:: ============================================
echo.
echo [Part 6] Verify Restoration
echo --------------------------------------------

if exist HWIDTest.exe (
    echo   Running HWIDTest.exe after restore...
    HWIDTest.exe > _restored.txt 2>&1
    
    :: Compare with original
    echo   Comparing MachineGuid (should match original):
    findstr /C:"MachineGuid" _before_spoof.txt
    findstr /C:"MachineGuid" _restored.txt
)

:: ============================================
:: Summary
:: ============================================
echo.
echo ============================================
echo   Test Results: %PASS% passed, %FAIL% failed
echo ============================================

:: Cleanup
del _test_config.cfg 2>nul
del _before_spoof.txt 2>nul
del _after_spoof.txt 2>nul
del _restored.txt 2>nul
del _reg_output.txt 2>nul
del _restore_output.txt 2>nul

if %FAIL% equ 0 (
    echo   All tests passed!
    echo.
    exit /b 0
) else (
    echo   Some tests failed!
    echo.
    exit /b 1
)

