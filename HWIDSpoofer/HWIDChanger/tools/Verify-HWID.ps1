# Read-only HWID report. No registry writes, hooks, driver loads or device restarts.
[CmdletBinding()]
param(
    [ValidateSet('Before','After','Restored')]
    [string]$Phase = 'Before',
    [string]$ReportDir = (Join-Path $env:USERPROFILE 'Desktop\HWID-Verification')
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Path $ReportDir -Force | Out-Null
function Read-Value([scriptblock]$Block) {
    try {
        $value = & $Block
        if ($null -eq $value) { return '[unavailable]' }
        $s = ([string]$value).Trim()
        if (-not $s) { return '[unavailable]' }
        return $s
    } catch { return '[unavailable]' }
}
$sample = [ordered]@{
    Timestamp        = (Get-Date).ToString('o')
    MachineGuid      = Read-Value { (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Cryptography').MachineGuid }
    ProductId        = Read-Value { (Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion').ProductId }
    ComputerNameKey  = Read-Value { (Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\ComputerName\ComputerName').ComputerName }
    ActiveName       = Read-Value { [Environment]::MachineName }
    BiosSerial       = Read-Value { (Get-CimInstance Win32_BIOS).SerialNumber }
    MotherboardSerial= Read-Value { (Get-CimInstance Win32_BaseBoard).SerialNumber }
    DiskSerial       = Read-Value { (Get-CimInstance Win32_DiskDrive | Select-Object -First 1).SerialNumber }
    VolumeSerial     = Read-Value { (Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='C:'").VolumeSerialNumber }
    MacAddress       = Read-Value { (Get-NetAdapter | Where-Object Status -eq 'Up' | Select-Object -First 1).MacAddress }
}
$path = Join-Path $ReportDir ("$Phase.json")
$sample | ConvertTo-Json -Depth 3 | Set-Content -Encoding UTF8 $path
Write-Host "Saved $Phase measurements: $path"
if ($Phase -eq 'Before') {
    Write-Host 'Real values currently reported by Windows (read-only):'
    $sample.GetEnumerator() | Format-Table -AutoSize -Wrap
}
if ($Phase -eq 'After' -or $Phase -eq 'Restored') {
    $baselinePath = Join-Path $ReportDir 'Before.json'
    if (-not (Test-Path $baselinePath)) {
        Write-Warning 'No Before.json exists; run -Phase Before first.'
        exit 2
    }
    $old = Get-Content $baselinePath -Raw | ConvertFrom-Json
    $rows = foreach ($property in @('MachineGuid','ProductId','ComputerNameKey','ActiveName','BiosSerial','MotherboardSerial','DiskSerial','VolumeSerial','MacAddress')) {
        $a = [string]$old.$property
        $b = [string]$sample[$property]
        $state = if ($a -eq '[unavailable]' -or $b -eq '[unavailable]') { 'NOT VERIFIED' }
                 elseif ($a -eq $b) { 'UNCHANGED' }
                 else { 'CHANGED' }
        [PSCustomObject]@{ Identifier=$property; Result=$state; Before=$a; After=$b }
    }
    $rows | Format-Table -AutoSize -Wrap
    $csvName = if ($Phase -eq 'Restored') { 'Restore-Comparison.csv' } else { 'Comparison.csv' }
    $rows | Export-Csv -NoTypeInformation -Encoding UTF8 (Join-Path $ReportDir $csvName)
    if ($Phase -eq 'Restored') {
        $notRestored = @($rows | Where-Object { $_.Result -eq 'CHANGED' })
        if ($notRestored.Count -gt 0) { Write-Warning "$($notRestored.Count) values differ from baseline after restore" }
        else { Write-Host 'Restore verification: All available readings match baseline.' }
    }
    Write-Host 'Read-only comparison complete. Registry and WMI/API readings are different views.'
}
