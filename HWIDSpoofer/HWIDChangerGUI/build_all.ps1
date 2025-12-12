# HWID Changer GUI - Build Script
# Run: powershell -ExecutionPolicy Bypass -File build_all.ps1

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  HWID Changer GUI - Build" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Step 1: Create icon
Write-Host "[1/4] Creating app icon..." -ForegroundColor Yellow
try {
    if (Test-Path "$scriptDir\create_icon.ps1") {
        & "$scriptDir\create_icon.ps1" 2>$null
        if (Test-Path "$scriptDir\app.ico") {
            Write-Host "  Done" -ForegroundColor Green
        } else {
            Write-Host "  Skipped (using system icon)" -ForegroundColor DarkGray
        }
    }
    else {
        Write-Host "  Skipped (no script)" -ForegroundColor DarkGray
    }
}
catch {
    Write-Host "  Skipped (using system icon)" -ForegroundColor DarkGray
}

# Step 2: Find MSBuild
Write-Host "[2/4] Finding compiler..." -ForegroundColor Yellow
$msbuildPaths = @(
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\MSBuild.exe",
    "C:\Windows\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
)

$msbuild = $null
foreach ($path in $msbuildPaths) {
    if (Test-Path $path) {
        $msbuild = $path
        break
    }
}

if (-not $msbuild) {
    Write-Host "  ERROR: MSBuild.exe not found" -ForegroundColor Red
    Write-Host "  Please install Visual Studio or .NET Framework SDK" -ForegroundColor Red
    exit 1
}

Write-Host "  Found: $msbuild" -ForegroundColor Green

# Step 3: Build project
Write-Host "[3/4] Building project..." -ForegroundColor Yellow
& $msbuild HWIDChangerGUI.csproj /p:Configuration=Release /p:Platform=AnyCPU /v:minimal /nologo

if ($LASTEXITCODE -ne 0) {
    Write-Host "  ERROR: Build failed" -ForegroundColor Red
    exit 1
}
Write-Host "  Build successful" -ForegroundColor Green

# Step 4: Package
Write-Host "[4/4] Creating release package..." -ForegroundColor Yellow

$releaseDir = "$scriptDir\bin\Release"
$outputDir = "$releaseDir\HWIDChanger"

# Create output directory
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

# Copy GUI
Copy-Item "$releaseDir\HWIDChangerGUI.exe" "$outputDir\" -Force
if (Test-Path "$releaseDir\HWIDChangerGUI.exe.config") {
    Copy-Item "$releaseDir\HWIDChangerGUI.exe.config" "$outputDir\" -Force
}

# Copy core files
$coreBin = "$scriptDir\..\HWIDChanger\bin"
$coreFiles = @(
    "HWIDChanger.exe",
    "HWIDChanger.dll",
    "HWIDTest.exe",
    "README.md"
)

foreach ($file in $coreFiles) {
    $src = Join-Path $coreBin $file
    if (Test-Path $src) {
        Copy-Item $src "$outputDir\" -Force
        Write-Host "  + Copied $file" -ForegroundColor DarkGray
    }
}

Write-Host "  Package complete" -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  BUILD COMPLETE!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Output: $outputDir" -ForegroundColor White
Write-Host ""
Write-Host "Files:" -ForegroundColor White
Get-ChildItem $outputDir | ForEach-Object {
    $size = if ($_.Length -gt 1MB) { "{0:N2} MB" -f ($_.Length / 1MB) } else { "{0:N0} KB" -f ($_.Length / 1KB) }
    Write-Host ("  {0,-25} {1,10}" -f $_.Name, $size) -ForegroundColor Gray
}
Write-Host ""
