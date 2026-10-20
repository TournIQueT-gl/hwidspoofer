# MAC address override for a disposable Windows Sandbox session only.
# Uses the adapter driver's supported Set-NetAdapter -MacAddress setting.
# Originals are saved BEFORE modification and verified on restore.
[CmdletBinding()]
param([ValidateSet('Apply','Restore','Status','Test')] [string]$Action='Status')
$ErrorActionPreference='Stop'
$stateDir=Join-Path $env:LOCALAPPDATA 'HWIDChanger'
$stateFile=Join-Path $stateDir 'mac-original.json'
function CurrentMac([string]$name) {
    return (Get-NetAdapter -Name $name -ErrorAction Stop).MacAddress
}
function RestoreMac {
    if (-not (Test-Path -LiteralPath $stateFile)) {
        Write-Host 'No saved original MAC. Nothing changed by this feature.'
        return 1
    }
    $state=Get-Content -LiteralPath $stateFile -Raw | ConvertFrom-Json
    if (-not $state.Name -or $state.Original -notmatch '^([0-9A-F]{2}-){5}[0-9A-F]{2}$') {
        Write-Error 'Invalid backup. Refusing to change adapter.'
        return 1
    }
    Write-Host ('Restoring '+$state.Name+' to '+$state.Original)
    Set-NetAdapter -Name $state.Name -MacAddress $state.Original -Confirm:$false -ErrorAction Stop
    Start-Sleep -Seconds 2
    $actual=CurrentMac $state.Name
    if ($actual -ne $state.Original) {
        Write-Warning ('Restore not confirmed. Actual: '+$actual+'. Preserve '+$stateFile)
        return 1
    }
    Remove-Item -LiteralPath $stateFile -Force
    Write-Host ('RESTORE PASS: '+$actual)
    return 0
}
try {
    if ($env:USERNAME -ne 'WDAGUtilityAccount') {
        throw 'Experimental MAC override is restricted to Windows Sandbox.'
    }
    if ($Action -eq 'Restore') { exit (RestoreMac) }
    if ($Action -eq 'Status') {
        if (Test-Path -LiteralPath $stateFile) {
            $state=Get-Content -LiteralPath $stateFile -Raw | ConvertFrom-Json
            Write-Host ('Saved original: '+$state.Name+' / '+$state.Original)
            Write-Host ('Current:        '+(CurrentMac $state.Name))
        } else { Write-Host 'No MAC override backup exists.' }
        exit 0
    }
    if (Test-Path -LiteralPath $stateFile) {
        throw 'Previous MAC backup exists. Restore it first; refusing to overwrite.'
    }
    $adapter=Get-NetAdapter | Where-Object { $_.Status -eq 'Up' -and $_.MacAddress } | Select-Object -First 1
    if (-not $adapter) { throw 'No active adapter with a MAC address.' }
    $rng=[System.Security.Cryptography.RandomNumberGenerator]::Create()
    $random=New-Object byte[] 5
    try { $rng.GetBytes($random) } finally { $rng.Dispose() }
    $new='02-'+(($random | ForEach-Object { $_.ToString('X2') }) -join '-')
    New-Item -ItemType Directory -Path $stateDir -Force | Out-Null
    $state=[ordered]@{ Name=$adapter.Name; Original=$adapter.MacAddress; Requested=$new; Time=(Get-Date).ToString('o') }
    $json=$state | ConvertTo-Json -Compress
    $file=[System.IO.File]::Open($stateFile,[System.IO.FileMode]::CreateNew)
    try {
        $bytes=[System.Text.Encoding]::UTF8.GetBytes($json)
        $file.Write($bytes,0,$bytes.Length)
        $file.Flush()
    } finally { $file.Dispose() }
    Write-Host ('Adapter: '+$adapter.Name)
    Write-Host ('Original: '+$adapter.MacAddress)
    Write-Host ('Requested: '+$new)
    $testPassed=$false
    try {
        Set-NetAdapter -Name $adapter.Name -MacAddress $new -Confirm:$false -ErrorAction Stop
        Start-Sleep -Seconds 2
        $current=CurrentMac $adapter.Name
        Write-Host ('Readback: '+$current)
        $testPassed=($current -eq $new)
        if (-not $testPassed) { Write-Warning 'Adapter did not report requested MAC.' }
    } catch {
        Write-Warning ('Apply failed: '+$_.Exception.Message)
    }
    if ($Action -eq 'Test' -or -not $testPassed) {
        $restored=RestoreMac
        if ($restored -ne 0) { exit 1 }
        if ($testPassed) { Write-Host 'TEST PASS: changed and restored.'; exit 0 }
        exit 1
    }
    Write-Host ('APPLY PASS: '+$current)
    Write-Host 'To revert, choose Restore MAC in the menu or use --mac-restore.'
    exit 0
} catch {
    Write-Host ('ERROR: '+$_.Exception.Message) -ForegroundColor Red
    exit 1
}
