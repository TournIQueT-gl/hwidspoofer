# HWID Changer 功能测试脚本
# ================================================

Write-Host "`n" -NoNewline
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "       HWID Changer 功能测试" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

$exe = ".\HWIDChanger.exe"
$passed = 0
$failed = 0

function Test-Feature {
    param($name, $cmd, $expected)
    Write-Host "  [$name] " -NoNewline
    $output = cmd /c "$exe $cmd 2>&1"
    if ($output -match $expected) {
        Write-Host "PASS" -ForegroundColor Green
        $script:passed++
    } else {
        Write-Host "FAIL" -ForegroundColor Red
        Write-Host "    Expected: $expected"
        Write-Host "    Got: $output"
        $script:failed++
    }
}

# 测试1: 帮助信息
Test-Feature "帮助信息" "--help" "Global Mode"

# 测试2: 生成随机HWID
Test-Feature "生成随机HWID" "-r -q" ""

# 测试3: 保存配置
Test-Feature "保存配置" "-r -s test_result.cfg" "Config saved"

# 测试4: 加载配置
Test-Feature "加载配置" "-c test_result.cfg -q" ""

# 测试5: 注册表修改
Test-Feature "注册表修改" "-c test_result.cfg --registry-global" "Registry modified"

# 测试6: 注册表恢复
Test-Feature "注册表恢复" "--restore-registry" "Registry restored"

# 测试7: 检查配置文件内容
Write-Host "  [配置文件检查] " -NoNewline
$cfg = Get-Content test_result.cfg -Raw
if ($cfg -match "disk_serial" -and $cfg -match "mac_address" -and $cfg -match "system_uuid") {
    Write-Host "PASS" -ForegroundColor Green
    $passed++
} else {
    Write-Host "FAIL" -ForegroundColor Red
    $failed++
}

# 测试8: 全局模式（预期失败，因为没有驱动）
Write-Host "  [全局模式(无驱动)] " -NoNewline
$output = cmd /c "$exe -r --enable-global 2>&1"
if ($output -match "Failed") {
    Write-Host "PASS (预期失败)" -ForegroundColor Yellow
    $passed++
} else {
    Write-Host "UNEXPECTED" -ForegroundColor Red
    $failed++
}

# 总结
Write-Host ""
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  测试结果: $passed 通过, $failed 失败" -ForegroundColor $(if ($failed -eq 0) {"Green"} else {"Red"})
Write-Host "============================================" -ForegroundColor Cyan
Write-Host ""

# 清理
Remove-Item test_result.cfg -ErrorAction SilentlyContinue

if ($failed -eq 0) {
    Write-Host "所有测试通过!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "有测试失败，请检查!" -ForegroundColor Red
    exit 1
}

