# Generates placeholder 64x64 sprite PNGs into assets/sprites using System.Drawing.
# Replace these files with real artwork later — the engine loads whatever PNGs are here.
Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot "..\assets\sprites"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

function New-Canvas {
    $bmp = New-Object System.Drawing.Bitmap 64,64
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)
    return $bmp,$g
}
function Save-Canvas($bmp,$g,$name) {
    $g.Dispose()
    $path = Join-Path $outDir $name
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "wrote $name"
}
function RGB($r,$g,$b,$a=255) { [System.Drawing.Color]::FromArgb($a,$r,$g,$b) }
function Brush($c) { New-Object System.Drawing.SolidBrush $c }
function Pen($c,$w=2) { New-Object System.Drawing.Pen $c,$w }

# ---- floor (opaque tile) ----
$bmp,$g = New-Canvas
$g.FillRectangle((Brush (RGB 38 40 54)), 0,0,64,64)
$g.DrawRectangle((Pen (RGB 50 53 70) 2), 1,1,61,61)
$g.FillRectangle((Brush (RGB 44 46 62)), 6,6,6,6)
$g.FillRectangle((Brush (RGB 44 46 62)), 52,52,6,6)
Save-Canvas $bmp $g "floor.png"

# ---- wall (opaque brick) ----
$bmp,$g = New-Canvas
$g.FillRectangle((Brush (RGB 78 80 104)), 0,0,64,64)
$mortar = Pen (RGB 40 42 56) 3
for ($y=0; $y -le 64; $y+=21) { $g.DrawLine($mortar, 0,$y,64,$y) }
$g.DrawLine($mortar, 32,0,32,21)
$g.DrawLine($mortar, 10,21,10,42); $g.DrawLine($mortar, 48,21,48,42)
$g.DrawLine($mortar, 32,42,32,64)
$g.FillRectangle((Brush (RGB 96 99 126)), 0,0,64,4)   # top bevel
Save-Canvas $bmp $g "wall.png"

# ---- player (blue astronaut) ----
$bmp,$g = New-Canvas
$g.FillEllipse((Brush (RGB 40 90 180)), 14,30,36,30)         # body
$g.FillEllipse((Brush (RGB 80 140 230)), 16,6,32,32)         # helmet
$g.FillEllipse((Brush (RGB 25 35 60)), 22,14,20,16)          # visor
$g.FillEllipse((Brush (RGB 120 200 255)), 26,17,7,5)         # visor glint
$g.DrawEllipse((Pen (RGB 150 200 255) 2), 16,6,32,32)
Save-Canvas $bmp $g "player.png"

# ---- player dead (gray) ----
$bmp,$g = New-Canvas
$g.FillEllipse((Brush (RGB 120 120 130)), 16,12,32,32)       # skull
$g.FillEllipse((Brush (RGB 30 30 36)), 22,22,8,8)            # eye
$g.FillEllipse((Brush (RGB 30 30 36)), 34,22,8,8)            # eye
$g.FillRectangle((Brush (RGB 90 90 100)), 20,44,24,8)        # jaw
Save-Canvas $bmp $g "player_dead.png"

# ---- grunt (light/grayscale so it tints cleanly) ----
$bmp,$g = New-Canvas
$g.FillEllipse((Brush (RGB 225 225 225)), 8,14,48,46)        # blob body
$g.FillEllipse((Brush (RGB 40 40 40)), 20,28,9,9)            # eye
$g.FillEllipse((Brush (RGB 40 40 40)), 35,28,9,9)            # eye
$pts = @(
  (New-Object System.Drawing.Point 20,46),
  (New-Object System.Drawing.Point 26,40),
  (New-Object System.Drawing.Point 32,46),
  (New-Object System.Drawing.Point 38,40),
  (New-Object System.Drawing.Point 44,46)
)
$g.DrawLines((Pen (RGB 40 40 40) 2), $pts)                   # jagged mouth
Save-Canvas $bmp $g "grunt.png"

# ---- chest (gold object) ----
$bmp,$g = New-Canvas
$g.FillRectangle((Brush (RGB 110 72 36)), 10,28,44,28)       # body
$g.FillPie((Brush (RGB 88 56 28)), 10,14,44,32, 180, 180)    # lid
$g.FillRectangle((Brush (RGB 240 196 70)), 10,34,44,5)       # band
$g.FillEllipse((Brush (RGB 250 210 90)), 28,34,8,10)         # lock
$g.DrawRectangle((Pen (RGB 60 40 20) 2), 10,28,44,28)
Save-Canvas $bmp $g "chest.png"

# ---- fire rune: 3 animation frames (orange, varying glow) ----
$frame = 0
foreach ($spec in @(@(16,180),@(10,255),@(22,210))) {
    $inset = $spec[0]; $alpha = $spec[1]
    $bmp,$g = New-Canvas
    $g.FillEllipse((Brush (RGB 120 40 0 90)), 4,4,56,56)                 # outer halo
    $g.FillEllipse((Brush (RGB 255 140 30 $alpha)), $inset,$inset,(64-2*$inset),(64-2*$inset))
    $g.FillEllipse((Brush (RGB 255 230 150 $alpha)), 26,26,12,12)        # hot core
    $g.DrawEllipse((Pen (RGB 255 90 10) 2), 8,8,48,48)                    # ring
    Save-Canvas $bmp $g ("firerune_{0}.png" -f $frame)
    $frame++
}

Write-Host "done -> $outDir"
