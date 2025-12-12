# HWID Changer 深度测试脚本
# ================================================

$ErrorActionPreference = "Continue"
$pass = 0
$fail = 0

function Test-Result($name, $condition, $detail = "") {
    if ($condition) {
        Write-Host "  [PASS] $name" -ForegroundColor Green
        if ($detail) { Write-Host "         $detail" -ForegroundColor Gray }
        $script:pass++
    } else {
        Write-Host "  [FAIL] $name" -ForegroundColor Red
        if ($detail) { Write-Host "         $detail" -ForegroundColor Yellow }
        $script:fail++
    }
}

Write-Host "`n" -NoNewline
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  HWID Changer - Deep Testing Suite" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host ""

# ================================================================
# 测试1: 记录原始值
# ================================================================
Write-Host "[Phase 1] Recording Original Values" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$origGuid = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid
$origProductId = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion").ProductId
$origDisk = (Get-WmiObject Win32_DiskDrive | Select-Object -First 1).SerialNumber
$origBios = (Get-WmiObject Win32_BIOS).SerialNumber
$origUuid = (Get-WmiObject Win32_ComputerSystemProduct).UUID
$origMac = (Get-NetAdapter | Where-Object Status -eq 'Up' | Select-Object -First 1).MacAddress

Write-Host "  MachineGuid:  $origGuid"
Write-Host "  ProductId:    $origProductId"
Write-Host "  Disk Serial:  $origDisk"
Write-Host "  BIOS Serial:  $origBios"
Write-Host "  System UUID:  $origUuid"
Write-Host "  MAC Address:  $origMac"
Write-Host ""

# ================================================================
# 测试2: 应用全系统伪装
# ================================================================
Write-Host "[Phase 2] Applying Full Usermode Spoof" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$output = & .\HWIDChanger.exe -r -s deep_test.cfg --full-usermode 2>&1
$output | ForEach-Object { Write-Host "  $_" }
Write-Host ""

# ================================================================
# 测试3: 验证注册表修改
# ================================================================
Write-Host "[Phase 3] Verifying Registry Modifications" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$newGuid = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid
$newProductId = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion").ProductId

Test-Result "MachineGuid changed" ($newGuid -ne $origGuid) "Was: $origGuid, Now: $newGuid"
Test-Result "ProductId changed" ($newProductId -ne $origProductId) "Was: $origProductId, Now: $newProductId"
Write-Host ""

# ================================================================
# 测试4: 多种系统工具验证
# ================================================================
Write-Host "[Phase 4] Multi-Tool Verification" -ForegroundColor Yellow
Write-Host "----------------------------------------"

# PowerShell Get-ItemProperty
$ps1 = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid
Write-Host "  PowerShell Get-ItemProperty: $ps1"
Test-Result "PowerShell sees spoofed value" ($ps1 -eq $newGuid)

# CMD reg query
$cmd1 = cmd /c "reg query `"HKLM\SOFTWARE\Microsoft\Cryptography`" /v MachineGuid" 2>&1 | Select-String "REG_SZ"
Write-Host "  CMD reg query: $($cmd1 -replace '.*REG_SZ\s+', '')"
Test-Result "CMD sees spoofed value" ($cmd1 -match $newGuid)

# .NET Registry
$dotnet1 = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Microsoft\Cryptography").GetValue("MachineGuid")
Write-Host "  .NET Registry: $dotnet1"
Test-Result ".NET sees spoofed value" ($dotnet1 -eq $newGuid)

Write-Host ""

# ================================================================
# 测试5: WMI查询测试（这些需要内核驱动才能完全伪装）
# ================================================================
Write-Host "[Phase 5] WMI Query Test (Info Only)" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$wmiDisk = (Get-WmiObject Win32_DiskDrive | Select-Object -First 1).SerialNumber
$wmiBios = (Get-WmiObject Win32_BIOS).SerialNumber
$wmiUuid = (Get-WmiObject Win32_ComputerSystemProduct).UUID

Write-Host "  WMI Disk Serial: $wmiDisk"
Write-Host "  WMI BIOS Serial: $wmiBios"
Write-Host "  WMI System UUID: $wmiUuid"
Write-Host ""
Write-Host "  Note: WMI queries read from hardware directly." -ForegroundColor DarkGray
Write-Host "        Registry spoof does NOT affect WMI." -ForegroundColor DarkGray
Write-Host "        Use process injection (-n) for WMI spoof." -ForegroundColor DarkGray
Write-Host ""

# ================================================================
# 测试6: HWIDTest.exe 验证
# ================================================================
Write-Host "[Phase 6] HWIDTest.exe Verification" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$testOutput = & .\HWIDTest.exe 2>&1 | Out-String
$testGuid = if ($testOutput -match "MachineGuid:\s+(\S+)") { $matches[1] } else { "NOT_FOUND" }
$testProductId = if ($testOutput -match "ProductId:\s+(\S+)") { $matches[1] } else { "NOT_FOUND" }

Write-Host "  HWIDTest MachineGuid: $testGuid"
Write-Host "  HWIDTest ProductId:   $testProductId"
Test-Result "HWIDTest sees spoofed MachineGuid" ($testGuid -eq $newGuid)
Write-Host ""

# ================================================================
# 测试7: AppInit状态检查
# ================================================================
Write-Host "[Phase 7] AppInit Status Check" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$appInitDlls = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows" -ErrorAction SilentlyContinue).AppInit_DLLs
$loadAppInit = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows" -ErrorAction SilentlyContinue).LoadAppInit_DLLs
$requireSigned = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows" -ErrorAction SilentlyContinue).RequireSignedAppInit_DLLs

Write-Host "  AppInit_DLLs: $appInitDlls"
Write-Host "  LoadAppInit_DLLs: $loadAppInit"
Write-Host "  RequireSignedAppInit_DLLs: $requireSigned"

Test-Result "AppInit DLL path set" ($appInitDlls -like "*HWIDChanger.dll*")
Test-Result "LoadAppInit enabled" ($loadAppInit -eq 1)
Test-Result "Signature requirement disabled" ($requireSigned -eq 0)
Write-Host ""

# ================================================================
# 测试8: 配置文件完整性
# ================================================================
Write-Host "[Phase 8] Config File Integrity" -ForegroundColor Yellow
Write-Host "----------------------------------------"

if (Test-Path "deep_test.cfg") {
    $cfg = Get-Content "deep_test.cfg" -Raw
    Test-Result "Config has disk_serial" ($cfg -match "disk_serial")
    Test-Result "Config has machine_guid" ($cfg -match "machine_guid")
    Test-Result "Config has mac_address" ($cfg -match "mac_address")
    Test-Result "Config has system_uuid" ($cfg -match "system_uuid")
    Test-Result "Config has volume_serial" ($cfg -match "volume_serial")
} else {
    Write-Host "  [FAIL] Config file not found" -ForegroundColor Red
    $fail += 5
}
Write-Host ""

# ================================================================
# 测试9: 恢复功能
# ================================================================
Write-Host "[Phase 9] Restore Functionality" -ForegroundColor Yellow
Write-Host "----------------------------------------"

& .\HWIDChanger.exe --restore-registry 2>&1 | ForEach-Object { Write-Host "  $_" }
& .\HWIDChanger.exe --disable-appinit 2>&1 | ForEach-Object { Write-Host "  $_" }

$restoredGuid = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid
$restoredAppInit = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Windows" -ErrorAction SilentlyContinue).LoadAppInit_DLLs

Test-Result "Registry restored" ($restoredGuid -eq $origGuid -or $restoredGuid -ne $newGuid) "Current: $restoredGuid"
Test-Result "AppInit disabled" ($restoredAppInit -eq 0 -or $restoredAppInit -eq $null)
Write-Host ""

# ================================================================
# 测试10: 清理验证
# ================================================================
Write-Host "[Phase 10] Final State Verification" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$finalGuid = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid
Write-Host "  Final MachineGuid: $finalGuid"
Write-Host "  Spoof is: $(if ($finalGuid -eq $newGuid) { 'STILL ACTIVE' } else { 'REMOVED' })"
Write-Host ""

# ================================================================
# 清理
# ================================================================
Remove-Item "deep_test.cfg" -ErrorAction SilentlyContinue

# ================================================================
# 总结
# ================================================================
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  DEEP TEST SUMMARY" -ForegroundColor Cyan
Write-Host "================================================================" -ForegroundColor Cyan
Write-Host "  Passed: $pass" -ForegroundColor $(if ($pass -gt 0) { "Green" } else { "White" })
Write-Host "  Failed: $fail" -ForegroundColor $(if ($fail -gt 0) { "Red" } else { "White" })
Write-Host "================================================================" -ForegroundColor Cyan

if ($fail -eq 0) {
    Write-Host "`n  [SUCCESS] All deep tests passed!" -ForegroundColor Green
} else {
    Write-Host "`n  [WARNING] Some tests failed!" -ForegroundColor Yellow
}
Write-Host ""

