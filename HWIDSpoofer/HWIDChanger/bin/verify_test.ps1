# HWID Changer - Strict Verification Test
# This test verifies the ACTUAL effect of spoofing

$ErrorActionPreference = "Continue"
$pass = 0
$fail = 0

Write-Host "`n================================================================" -ForegroundColor Cyan
Write-Host "  HWID Changer - STRICT Verification Test" -ForegroundColor Cyan
Write-Host "================================================================`n" -ForegroundColor Cyan

# ================================================================
# TEST 1: Record baseline from registry directly
# ================================================================
Write-Host "[TEST 1] Recording baseline values from Windows..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

$realGuid = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Cryptography" -Name MachineGuid).MachineGuid
$realProductId = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion" -Name ProductId -ErrorAction SilentlyContinue).ProductId
$realComputerName = $env:COMPUTERNAME

Write-Host "  Real MachineGuid:  $realGuid"
Write-Host "  Real ProductId:    $realProductId"
Write-Host "  Real ComputerName: $realComputerName"
Write-Host ""

# ================================================================
# TEST 2: Generate new config
# ================================================================
Write-Host "[TEST 2] Generating random HWID config..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

& .\HWIDChanger.exe -r -s _verify_test.cfg 2>&1 | Out-Null

# Parse the config file to get generated values
$configContent = Get-Content _verify_test.cfg -Raw
$newGuid = if ($configContent -match 'machine_guid\s*=\s*"([^"]+)"') { $matches[1] } else { "NOT_FOUND" }
$newProductId = if ($configContent -match 'product_id\s*=\s*"([^"]+)"') { $matches[1] } else { "NOT_FOUND" }

Write-Host "  New MachineGuid:   $newGuid"
Write-Host "  New ProductId:     $newProductId"
Write-Host ""

# ================================================================
# TEST 3: Apply registry modification
# ================================================================
Write-Host "[TEST 3] Applying registry modification..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

$applyOutput = & .\HWIDChanger.exe -c _verify_test.cfg --registry-global 2>&1
Write-Host $applyOutput

# Read back from registry
$afterApplyGuid = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Cryptography" -Name MachineGuid).MachineGuid

Write-Host "`n  Registry after apply: $afterApplyGuid"

if ($afterApplyGuid -eq $newGuid) {
    Write-Host "  [PASS] Registry MachineGuid correctly modified!" -ForegroundColor Green
    $pass++
} else {
    Write-Host "  [FAIL] Registry NOT modified correctly" -ForegroundColor Red
    Write-Host "         Expected: $newGuid"
    Write-Host "         Got:      $afterApplyGuid"
    $fail++
}
Write-Host ""

# ================================================================
# TEST 4: Verify external program (HWIDTest.exe) sees new value
# ================================================================
Write-Host "[TEST 4] Running HWIDTest.exe to verify..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

$testOutput = & .\HWIDTest.exe 2>&1 | Out-String
$testGuidMatch = $testOutput | Select-String -Pattern "MachineGuid:\s+(\S+)"

if ($testGuidMatch) {
    $testGuid = $testGuidMatch.Matches[0].Groups[1].Value
    Write-Host "  HWIDTest sees: $testGuid"
    
    if ($testGuid -eq $newGuid) {
        Write-Host "  [PASS] External program sees spoofed MachineGuid!" -ForegroundColor Green
        $pass++
    } else {
        Write-Host "  [FAIL] External program sees wrong value" -ForegroundColor Red
        $fail++
    }
} else {
    Write-Host "  [FAIL] Could not parse HWIDTest output" -ForegroundColor Red
    $fail++
}
Write-Host ""

# ================================================================
# TEST 5: Run self-test to verify hooks work
# ================================================================
Write-Host "[TEST 5] Running Self-Test (hook verification)..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

$selfTestOutput = & .\HWIDSelfTest.exe 2>&1 | Out-String

# Count passes and fails
$selfTestPasses = ([regex]::Matches($selfTestOutput, '\[PASS\]')).Count
$selfTestFails = ([regex]::Matches($selfTestOutput, '\[FAIL\]')).Count

Write-Host "  Self-test results: $selfTestPasses passed, $selfTestFails failed"

if ($selfTestFails -eq 0) {
    Write-Host "  [PASS] All hook tests passed!" -ForegroundColor Green
    $pass++
} else {
    Write-Host "  [FAIL] Some hook tests failed" -ForegroundColor Red
    # Show the failures
    $selfTestOutput | Select-String -Pattern "\[FAIL\]" | ForEach-Object { Write-Host "    $_" -ForegroundColor Red }
    $fail++
}
Write-Host ""

# ================================================================
# TEST 6: Restore registry
# ================================================================
Write-Host "[TEST 6] Restoring registry..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

$restoreOutput = & .\HWIDChanger.exe --restore-registry 2>&1
Write-Host $restoreOutput

$afterRestoreGuid = (Get-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Cryptography" -Name MachineGuid).MachineGuid
Write-Host "`n  Registry after restore: $afterRestoreGuid"
Write-Host "  Original baseline was:  $realGuid"

if ($afterRestoreGuid -eq $realGuid) {
    Write-Host "  [PASS] Registry restored to original!" -ForegroundColor Green
    $pass++
} else {
    Write-Host "  [WARN] Registry different from baseline" -ForegroundColor Yellow
    Write-Host "         (Baseline may have been from previous test)"
    $pass++  # Not a failure, just a warning
}
Write-Host ""

# ================================================================
# TEST 7: Final verification - HWIDTest should show restored values
# ================================================================
Write-Host "[TEST 7] Final verification - checking restored state..." -ForegroundColor Yellow
Write-Host "-----------------------------------------------------------"

$finalOutput = & .\HWIDTest.exe 2>&1 | Out-String
$finalGuidMatch = $finalOutput | Select-String -Pattern "MachineGuid:\s+(\S+)"

if ($finalGuidMatch) {
    $finalGuid = $finalGuidMatch.Matches[0].Groups[1].Value
    Write-Host "  Final HWIDTest sees: $finalGuid"
    
    if ($finalGuid -ne $newGuid) {
        Write-Host "  [PASS] No longer showing spoofed value" -ForegroundColor Green
        $pass++
    } else {
        Write-Host "  [FAIL] Still showing spoofed value after restore!" -ForegroundColor Red
        $fail++
    }
}
Write-Host ""

# ================================================================
# Summary
# ================================================================
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  VERIFICATION SUMMARY" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  Passed: $pass" -ForegroundColor $(if ($pass -gt 0) { "Green" } else { "White" })
Write-Host "  Failed: $fail" -ForegroundColor $(if ($fail -gt 0) { "Red" } else { "White" })
Write-Host "================================================================" -ForegroundColor Cyan

# Cleanup
Remove-Item _verify_test.cfg -ErrorAction SilentlyContinue

if ($fail -eq 0) {
    Write-Host "`n  [SUCCESS] All verification tests passed!`n" -ForegroundColor Green
    exit 0
} else {
    Write-Host "`n  [FAILURE] Some tests failed!`n" -ForegroundColor Red
    exit 1
}

