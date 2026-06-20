Add-Type -AssemblyName System.Drawing

$outDir = Join-Path $PSScriptRoot "..\assets\sprites"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$SIZE = 64
$GRID = 16
$CELL = $SIZE / $GRID

$palette = New-Object 'System.Collections.Generic.Dictionary[char,System.Drawing.Color]'
$palette.Add([char]'X', [System.Drawing.Color]::FromArgb(255, 26, 22, 30))
$palette.Add([char]'a', [System.Drawing.Color]::FromArgb(255, 42, 44, 60))
$palette.Add([char]'b', [System.Drawing.Color]::FromArgb(255, 50, 52, 70))
$palette.Add([char]'c', [System.Drawing.Color]::FromArgb(255, 34, 36, 50))
$palette.Add([char]'d', [System.Drawing.Color]::FromArgb(255, 92, 96, 120))
$palette.Add([char]'e', [System.Drawing.Color]::FromArgb(255, 70, 74, 96))
$palette.Add([char]'f', [System.Drawing.Color]::FromArgb(255, 112, 116, 142))
$palette.Add([char]'m', [System.Drawing.Color]::FromArgb(255, 40, 42, 56))
$palette.Add([char]'s', [System.Drawing.Color]::FromArgb(255, 232, 184, 144))
$palette.Add([char]'S', [System.Drawing.Color]::FromArgb(255, 198, 143, 102))
$palette.Add([char]'h', [System.Drawing.Color]::FromArgb(255, 74, 52, 30))
$palette.Add([char]'B', [System.Drawing.Color]::FromArgb(255, 41, 79, 120))
$palette.Add([char]'l', [System.Drawing.Color]::FromArgb(255, 58, 110, 165))
$palette.Add([char]'p', [System.Drawing.Color]::FromArgb(255, 54, 64, 79))
$palette.Add([char]'o', [System.Drawing.Color]::FromArgb(255, 32, 36, 44))
$palette.Add([char]'k', [System.Drawing.Color]::FromArgb(255, 20, 18, 26))
$palette.Add([char]'g', [System.Drawing.Color]::FromArgb(255, 225, 225, 225))
$palette.Add([char]'G', [System.Drawing.Color]::FromArgb(255, 180, 180, 180))
$palette.Add([char]'w', [System.Drawing.Color]::FromArgb(255, 110, 72, 36))
$palette.Add([char]'W', [System.Drawing.Color]::FromArgb(255, 80, 52, 26))
$palette.Add([char]'y', [System.Drawing.Color]::FromArgb(255, 240, 196, 70))
$palette.Add([char]'Y', [System.Drawing.Color]::FromArgb(255, 252, 224, 130))
$palette.Add([char]'r', [System.Drawing.Color]::FromArgb(255, 200, 60, 0))
$palette.Add([char]'O', [System.Drawing.Color]::FromArgb(255, 255, 140, 30))
$palette.Add([char]'F', [System.Drawing.Color]::FromArgb(255, 255, 200, 90))
$palette.Add([char]'C', [System.Drawing.Color]::FromArgb(255, 255, 244, 190))

function Save-Grid($rows, $name) {
    $bmp = New-Object System.Drawing.Bitmap $SIZE, $SIZE
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $g.Clear([System.Drawing.Color]::Transparent)
    for ($r = 0; $r -lt $GRID; $r++) {
        $line = if ($r -lt $rows.Count) { $rows[$r] } else { "" }
        $line = $line.PadRight($GRID, '.')
        for ($cc = 0; $cc -lt $GRID; $cc++) {
            $ch = $line[$cc]
            if ($palette.ContainsKey($ch)) {
                $brush = New-Object System.Drawing.SolidBrush $palette[$ch]
                $g.FillRectangle($brush, ($cc * $CELL), ($r * $CELL), $CELL, $CELL)
                $brush.Dispose()
            }
        }
    }
    $g.Dispose()
    $bmp.Save((Join-Path $outDir $name), [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "wrote $name"
}

$floor = @(
    'cccccccccccccccc',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caabaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaabaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaabaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'caaaaaaaaaaaaaac',
    'cccccccccccccccc'
)
Save-Grid $floor 'floor.png'

$wall = @(
    'ffffffffffffffff',
    'dddddddmdddddddd',
    'dddddddmdddddddd',
    'dddddddmdddddddd',
    'mmmmmmmmmmmmmmmm',
    'ddmdddddddddmddd',
    'ddmdddddddddmddd',
    'ddmdddddddddmddd',
    'mmmmmmmmmmmmmmmm',
    'dddddddmdddddddd',
    'dddddddmdddddddd',
    'dddddddmdddddddd',
    'mmmmmmmmmmmmmmmm',
    'ddmdddddddddmddd',
    'ddmdddddddddmddd',
    'ddmdddddddddmddd'
)
Save-Grid $wall 'wall.png'

$player = @(
    '................',
    '......hhhh......',
    '.....hhhhhh.....',
    '....hhhhhhhh....',
    '....hssssssh....',
    '....ssssssss....',
    '....ssksskss....',
    '....ssssssss....',
    '.....ssSSss.....',
    '...llllllllll...',
    '..sllllllllls..',
    '..sllllllllls..',
    '...llllllllll...',
    '...pppppppppp...',
    '....pp....pp....',
    '....oo....oo....'
)
Save-Grid $player 'player.png'

$playerDead = @(
    '................',
    '................',
    '.....gggggg.....',
    '....gggggggg....',
    '...gggggggggg...',
    '...gkkggggkkg...',
    '...gkkggggkkg...',
    '...gggggggggg...',
    '....ggGGGGgg....',
    '....gggggggg....',
    '....g.g.g.g.....',
    '................',
    '................',
    '................',
    '................',
    '................'
)
Save-Grid $playerDead 'player_dead.png'

$grunt = @(
    '................',
    '.....gggggg.....',
    '...gggggggggg...',
    '..gggggggggggg..',
    '..gggggggggggg..',
    '..ggkgggggkggg..',
    '..gggggggggggg..',
    '..ggGGGGGGGGgg..',
    '..gggggggggggg..',
    '..gggggggggggg..',
    '...gggggggggg...',
    '....gggggggg....',
    '.....gg..gg.....',
    '................',
    '................',
    '................'
)
Save-Grid $grunt 'grunt.png'

$chest = @(
    '................',
    '................',
    '...WWWWWWWWWW...',
    '..WwwwwwwwwwwW..',
    '..WyyyyyyyyyyW..',
    '..WwwwwwwwwwwW..',
    '..WwwwwYYwwwwW..',
    '..WwwwwYYwwwwW..',
    '..WwwwwwwwwwwW..',
    '..WwwwwwwwwwwW..',
    '..WWWWWWWWWWWW..',
    '................',
    '................',
    '................',
    '................',
    '................'
)
Save-Grid $chest 'chest.png'

$rune0 = @(
    '................',
    '................',
    '......rrrr......',
    '.....rOOOOr.....',
    '....rOOOOOOr....',
    '....rOOOOOOr....',
    '....rOOOOOOr....',
    '....rOOOOOOr....',
    '.....rOOOOr.....',
    '......rrrr......',
    '................',
    '................',
    '................',
    '................',
    '................',
    '................'
)
Save-Grid $rune0 'firerune_0.png'

$rune1 = @(
    '................',
    '......rrrr......',
    '....rrOOOOrr....',
    '...rOOOOOOOOr...',
    '..rOOOFFFFOOr..',
    '..rOOFFCCFFOOr.'.PadRight(16,'.'),
    '..rOFFCCCCFFOr.'.PadRight(16,'.'),
    '..rOFFCCCCFFOr.'.PadRight(16,'.'),
    '..rOOFFCCFFOOr.'.PadRight(16,'.'),
    '..rOOOFFFFOOr..',
    '...rOOOOOOOOr...',
    '....rrOOOOrr....',
    '......rrrr......',
    '................',
    '................',
    '................'
)
Save-Grid $rune1 'firerune_1.png'

$rune2 = @(
    '................',
    '................',
    '.....rrrrrr.....',
    '....rOOOOOOr....',
    '...rOOOFFOOOr...',
    '...rOOFCCFOOr...',
    '...rOFCCCCFOr...',
    '...rOFCCCCFOr...',
    '...rOOFCCFOOr...',
    '...rOOOFFOOOr...',
    '....rOOOOOOr....',
    '.....rrrrrr.....',
    '................',
    '................',
    '................',
    '................'
)
Save-Grid $rune2 'firerune_2.png'

Write-Host "done -> $outDir"
