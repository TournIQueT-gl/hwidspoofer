# HWID Test Script
Write-Host "====== Real Hardware Info ======" -ForegroundColor Cyan

# Disk Serial
Write-Host "`nDisk Serial:" -ForegroundColor Yellow
Get-WmiObject Win32_DiskDrive | Select SerialNumber | Format-List

# BIOS
Write-Host "BIOS Serial:" -ForegroundColor Yellow  
Get-WmiObject Win32_BIOS | Select SerialNumber | Format-List

# Baseboard
Write-Host "Baseboard Serial:" -ForegroundColor Yellow
Get-WmiObject Win32_BaseBoard | Select SerialNumber, Product | Format-List

# System UUID
Write-Host "System UUID:" -ForegroundColor Yellow
Get-WmiObject Win32_ComputerSystemProduct | Select UUID | Format-List

# MAC Address
Write-Host "MAC Address:" -ForegroundColor Yellow
Get-WmiObject Win32_NetworkAdapter | Where {$_.MACAddress} | Select Name, MACAddress | Format-Table

# Machine GUID
Write-Host "Machine GUID:" -ForegroundColor Yellow
(Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Cryptography").MachineGuid

# Volume Serial
Write-Host "`nVolume Serial (C:):" -ForegroundColor Yellow
$vol = Get-WmiObject Win32_LogicalDisk -Filter "DeviceID='C:'"
"0x{0:X8}" -f $vol.VolumeSerialNumber

Write-Host "`n================================" -ForegroundColor Cyan

