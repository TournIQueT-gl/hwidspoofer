# Read-only Windows guest verification for a configured QEMU hardware profile.
[CmdletBinding()]
param(
    [ValidatePattern('^[A-Za-z0-9_-]{1,30}$')][string]$SystemId='TEST-SYSTEM-001',
    [ValidatePattern('^[A-Za-z0-9_-]{1,30}$')][string]$BoardId='TEST-BOARD-001',
    [ValidatePattern('^[A-Za-z0-9_-]{1,20}$')][string]$DriveId='TEST-DISK-001'
)
$ErrorActionPreference='Stop'
function Probe([scriptblock]$script) {
    try {
        $value= & $script
        $v=[string]$value
        if ([string]::IsNullOrWhiteSpace($v)) { return $null }
        return $v.Trim()
    } catch { return $null }
}
$checks=@(
    [pscustomobject]@{
        Component='Win32_BIOS.SerialNumber'
        Expected=$SystemId
        Actual=(Probe { (Get-CimInstance Win32_BIOS).SerialNumber })
    },
    [pscustomobject]@{
        Component='SMBIOS system serial'
        Expected=$SystemId
        Actual=(Probe { (Get-CimInstance Win32_ComputerSystemProduct).IdentifyingNumber })
    },
    [pscustomobject]@{
        Component='SMBIOS baseboard serial'
        Expected=$BoardId
        Actual=(Probe { (Get-CimInstance Win32_BaseBoard).SerialNumber })
    },
    [pscustomobject]@{
        Component='Virtual disk serial'
        Expected=$DriveId
        Actual=(Probe { (Get-CimInstance Win32_DiskDrive | Select-Object -First 1).SerialNumber })
    }
)
$passed=0
foreach($check in $checks){
    $status='NOT VERIFIED'
    if($null -ne $check.Actual){
        if($check.Actual -eq $check.Expected){ $status='PASS'; $passed++ }
        else { $status='MISMATCH' }
    }
    $check | Add-Member -NotePropertyName Result -NotePropertyValue $status
}
$checks | Format-Table -AutoSize -Wrap
Write-Host ('Passed: '+$passed+'/4')
if ($passed -ne 4) { exit 1 }
Write-Host 'PASS: BIOS, system, baseboard and disk readings match the virtual hardware profile.'
