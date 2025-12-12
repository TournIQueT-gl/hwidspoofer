# HWID Changer 编译脚本 (PowerShell)

$ErrorActionPreference = "Stop"

Write-Host "`n========== HWID Changer Build ==========" -ForegroundColor Cyan

# VS路径
$vsPath = "C:\Program Files\Microsoft Visual Studio\18\Community"
$vcvars = "$vsPath\VC\Auxiliary\Build\vcvars64.bat"

# 创建输出目录
if (!(Test-Path "bin")) { New-Item -ItemType Directory -Path "bin" | Out-Null }

Write-Host "[*] Compiling..." -ForegroundColor Yellow

# 使用cmd调用vcvars并编译
$src = "main.c hwid_core.c hwid_hooks.c"
$out = "bin\HWIDChanger.exe"
$libs = "kernel32.lib advapi32.lib iphlpapi.lib user32.lib"

$buildCmd = @"
@echo off
call "$vcvars" >nul 2>&1
cd /d "$PWD"
cl /nologo /O2 /MT /W3 /D "WIN32_LEAN_AND_MEAN" /D "_CRT_SECURE_NO_WARNINGS" $src /Fe"$out" /link $libs
del *.obj 2>nul
"@

$buildCmd | Out-File -Encoding ASCII "temp_build.bat"
& cmd /c "temp_build.bat"
Remove-Item "temp_build.bat" -ErrorAction SilentlyContinue

if (Test-Path $out) {
    $size = (Get-Item $out).Length
    Write-Host "`n[OK] Build successful!" -ForegroundColor Green
    Write-Host "Output: $out" -ForegroundColor White
    Write-Host "Size: $size bytes ($([math]::Round($size/1024, 1)) KB)" -ForegroundColor White
} else {
    Write-Host "`n[ERROR] Build failed!" -ForegroundColor Red
}

Write-Host "========================================`n" -ForegroundColor Cyan

