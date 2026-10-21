# Unit-level preview check: does not start QEMU or require a disk image.
$ErrorActionPreference='Stop'
$scriptPath=Join-Path $PSScriptRoot '..\tools\VirtualHardwareProfile.ps1'
if (-not (Test-Path -LiteralPath $scriptPath)) { throw 'VirtualHardwareProfile.ps1 missing.' }
$fakeImage=[System.IO.Path]::Combine([System.IO.Path]::GetTempPath(),'virtual-hardware-guest.qcow2')
$preview=@(& $scriptPath -Image $fakeImage -SystemId 'SYS-1' -BoardId 'BOARD-1' -DriveId 'DISK-1') -join [Environment]::NewLine
foreach ($fragment in @(
  'SYS-1', 'BOARD-1', 'DISK-1',
  'type=1,serial=SYS-1', 'type=2,serial=BOARD-1',
  'ide-hd,drive=labdrive,serial=DISK-1',
  'Preview only'
)) {
    if (-not $preview.Contains($fragment)) { throw ('Preview missing: '+$fragment) }
}
if (-not $preview.Contains('-snapshot') -or -not $preview.Contains('-nic none')) {
    throw 'Preview missing isolation controls.'
}
Write-Output 'PASS: isolated QEMU hardware profile preview'
