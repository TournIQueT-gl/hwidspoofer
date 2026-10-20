# Integration smoke test for a disposable Windows Sandbox only.
# Builds fresh source; checks read-only hardware inspection and MAC apply/restore.
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
if ($env:USERNAME -ne 'WDAGUtilityAccount') { throw 'Run only inside Windows Sandbox.' }
$work=Join-Path $HOME 'Desktop\HWID-App-Check'
New-Item -ItemType Directory -Path $work -Force | Out-Null
$log=Join-Path $work 'smoke-result.txt'
Start-Transcript -Path $log -Force | Out-Null
try {
    $gcc='C:\msys64\ucrt64\bin\gcc.exe'
    if (-not (Test-Path $gcc)) {
        throw 'GCC missing inside Sandbox. Use the Sandbox where MSYS2 was installed.'
    }
    $env:Path=(Split-Path $gcc)+';'+$env:Path
    $archive=Join-Path $work 'latest.zip'
    $source=Join-Path $work 'source'
    Write-Host '[1/4] Downloading latest fix branch source...'
    Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/TournIQueT-gl/hwidspoofer/archive/refs/heads/fix/build-and-recovery-2026-10-21.zip' -OutFile $archive
    if (Test-Path $source) { Remove-Item $source -Recurse -Force }
    Expand-Archive -Path $archive -DestinationPath $source -Force
    $root=Get-ChildItem -Path $source -Directory | Select-Object -First 1
    if (-not $root) { throw 'Archive did not contain source folder.' }
    $project=Join-Path $root.FullName 'HWIDSpoofer\HWIDChanger'
    Write-Host '[2/4] Compiling Windows executable...'
    Push-Location $project
    try {
        & cmd.exe /d /c 'build.bat'
        if ($LASTEXITCODE -ne 0) { throw ('Build failed with code '+$LASTEXITCODE) }
    } finally { Pop-Location }
    $bin=Join-Path $project 'bin'
    $exe=Join-Path $bin 'HWIDChanger.exe'
    foreach($name in @('HWIDChanger.exe','MacOverride.ps1','Verify-HWID.ps1')) {
        if(-not(Test-Path(Join-Path $bin $name))) { throw ('Missing release file: '+$name) }
    }
    & $exe --help | Out-Null
    if ($LASTEXITCODE -ne 0) { throw '--help failed.' }
    Write-Host 'BUILD PASS'
    Write-Host '[3/4] Checking hardware display (former menu 5 crash)...'
    & $exe --show-hardware
    if ($LASTEXITCODE -ne 0) { throw ('Hardware display failed: '+$LASTEXITCODE) }
    Write-Host 'HARDWARE DISPLAY PASS'
    Write-Host '[4/4] MAC apply / readback / restore test...'
    $active=Get-NetAdapter | Where-Object { $_.Status -eq 'Up' -and $_.MacAddress } | Select-Object -First 1
    if (-not $active) { throw 'No active adapter found.' }
    $original=$active.MacAddress
    & $exe --mac-test
    $macCode=$LASTEXITCODE
    $restored=(Get-NetAdapter -Name $active.Name -ErrorAction Stop).MacAddress
    $backupFile=Join-Path $env:LOCALAPPDATA 'HWIDChanger\mac-original.json'
    Write-Host ('MAC TEST EXIT: '+$macCode)
    Write-Host ('MAC BEFORE: '+$original)
    Write-Host ('MAC AFTER:  '+$restored)
    if ($macCode -ne 0 -or $restored -ne $original -or (Test-Path $backupFile)) {
        Write-Warning 'MAC recovery incomplete or unsuccessful. Close Sandbox to discard changes.'
        throw 'MAC test did not pass.'
    }
    Write-Host 'MAC CHANGE AND RESTORE PASS'
    Write-Host 'ALL TESTS PASSED for the tested features.' -ForegroundColor Green
} catch {
    Write-Host ('FAIL: '+$_.Exception.Message) -ForegroundColor Red
} finally {
    Write-Host ('Log: '+$log)
    Stop-Transcript | Out-Null
}
