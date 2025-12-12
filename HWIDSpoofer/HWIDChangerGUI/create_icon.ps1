# Create simple HWID Changer icon
Add-Type -AssemblyName System.Drawing

$size = 64
$bitmap = New-Object System.Drawing.Bitmap($size, $size)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias

# Background - blue gradient
$brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
    (New-Object System.Drawing.Point(0, 0)),
    (New-Object System.Drawing.Point($size, $size)),
    [System.Drawing.Color]::FromArgb(59, 130, 246),
    [System.Drawing.Color]::FromArgb(37, 99, 235)
)

# Rounded rect
$radius = 12
$rect = New-Object System.Drawing.Rectangle(0, 0, $size - 1, $size - 1)
$path = New-Object System.Drawing.Drawing2D.GraphicsPath
$path.AddArc($rect.X, $rect.Y, $radius, $radius, 180, 90)
$path.AddArc($rect.X + $rect.Width - $radius, $rect.Y, $radius, $radius, 270, 90)
$path.AddArc($rect.X + $rect.Width - $radius, $rect.Y + $rect.Height - $radius, $radius, $radius, 0, 90)
$path.AddArc($rect.X, $rect.Y + $rect.Height - $radius, $radius, $radius, 90, 90)
$path.CloseFigure()
$graphics.FillPath($brush, $path)

# Shield icon
$shieldPen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 3)
$shieldBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)

# Draw shield shape
$shieldPath = New-Object System.Drawing.Drawing2D.GraphicsPath
$cx = $size / 2
$shieldPath.AddLine([float]($cx - 16), [float]14, [float]$cx, [float]8)
$shieldPath.AddLine([float]$cx, [float]8, [float]($cx + 16), [float]14)
$shieldPath.AddLine([float]($cx + 16), [float]14, [float]($cx + 16), [float]36)
$shieldPath.AddArc([float]($cx - 16), [float]28, 32, 24, 0, 180)
$shieldPath.CloseFigure()
$graphics.DrawPath($shieldPen, $shieldPath)

# Checkmark
$checkPen = New-Object System.Drawing.Pen([System.Drawing.Color]::White, 3)
$graphics.DrawLine($checkPen, [float]($cx - 8), [float]32, [float]($cx - 2), [float]38)
$graphics.DrawLine($checkPen, [float]($cx - 2), [float]38, [float]($cx + 10), [float]24)

$graphics.Dispose()

# Save as icon
$iconPath = Join-Path $PSScriptRoot "app.ico"
$hIcon = $bitmap.GetHicon()
$icon = [System.Drawing.Icon]::FromHandle($hIcon)

$fs = [System.IO.File]::Create($iconPath)
$icon.Save($fs)
$fs.Close()
$fs.Dispose()

$bitmap.Dispose()
Write-Host "Icon created: $iconPath"
