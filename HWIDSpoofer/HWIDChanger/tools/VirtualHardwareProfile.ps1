# Launch an isolated QEMU hardware-profile lab; the default is preview only.
# This file never modifies the host registry, firmware or physical drives.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Image,
    [ValidatePattern('^[A-Za-z0-9_-]{1,30}$')][string]$SystemId='TEST-SYSTEM-001',
    [ValidatePattern('^[A-Za-z0-9_-]{1,30}$')][string]$BoardId='TEST-BOARD-001',
    [ValidatePattern('^[A-Za-z0-9_-]{1,20}$')][string]$DriveId='TEST-DISK-001',
    [string]$Qemu='qemu-system-x86_64',
    [switch]$Run
)
$ErrorActionPreference='Stop'
if (-not [System.IO.Path]::IsPathRooted($Image)) { throw 'Use an absolute guest image path.' }
if ($Image.Contains(',')) { throw 'Guest image filename cannot contain a comma.' }
$format=switch ([System.IO.Path]::GetExtension($Image).ToLowerInvariant()) {
    '.qcow2' { 'qcow2' }
    '.raw' { 'raw' }
    '.img' { 'raw' }
    default { throw 'Image extension must be .qcow2, .raw, or .img.' }
}
$argsList=@(
    '-machine','pc',
    '-m','4096',
    '-smp','2',
    '-nic','none',
    '-snapshot',
    '-smbios',('type=1,serial='+$SystemId),
    '-smbios',('type=2,serial='+$BoardId),
    '-drive',('if=none,id=labdrive,format='+$format+',file='+$Image),
    '-device',('ide-hd,drive=labdrive,serial='+$DriveId)
)
Write-Output 'Virtual guest hardware configuration:'
Write-Output ('  System:      '+$SystemId)
Write-Output ('  Baseboard:   '+$BoardId)
Write-Output ('  IDE disk:    '+$DriveId)
Write-Output '  Network:     disabled'
Write-Output '  Disk writes: discarded when VM exits'
Write-Output '  Host hardware unchanged'
if (-not $Run) {
    Write-Output 'Preview only. Use -Run with QEMU installed and an existing OS image.'
    Write-Output ('QEMU arguments: '+($argsList -join ' '))
    return
}
if (-not (Test-Path -LiteralPath $Image -PathType Leaf)) {
    throw 'Guest image not found.'
}
$command=Get-Command $Qemu -CommandType Application -ErrorAction Stop
& $command.Source @argsList
if ($LASTEXITCODE -ne 0) { throw ('QEMU failed with exit code '+$LASTEXITCODE) }
