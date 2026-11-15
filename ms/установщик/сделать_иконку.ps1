Add-Type -AssemblyName System.Drawing

function New-IconBitmap([int]$размер) {
    $bmp = New-Object System.Drawing.Bitmap($размер, $размер)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::Transparent)

    $центр = $размер / 2.0
    $радиус = $размер * 0.48
    $круг = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 42, 44, 43))
    $g.FillEllipse($круг, [float]($центр - $радиус), [float]($центр - $радиус), [float]($радиус * 2), [float]($радиус * 2))
    $круг.Dispose()

    $толщина = [float]([Math]::Max(1.4, $размер * 0.085))
    $перо = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 241, 242, 242), $толщина)
    $перо.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
    $перо.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
    $перо.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round

    $точки = @(
        (New-Object System.Drawing.PointF([float]($размер * 0.320), [float]($размер * 0.289))),
        (New-Object System.Drawing.PointF([float]($размер * 0.484), [float]($размер * 0.430))),
        (New-Object System.Drawing.PointF([float]($размер * 0.320), [float]($размер * 0.570)))
    )
    $g.DrawLines($перо, $точки)
    $g.DrawLine($перо, [float]($размер * 0.5625), [float]($размер * 0.570), [float]($размер * 0.7266), [float]($размер * 0.570))

    $перо.Dispose()
    $g.Dispose()
    return $bmp
}

function Write-Ico([string]$путь, [int[]]$размеры) {
    $картинки = @()
    foreach ($р in $размеры) { $картинки += New-IconBitmap $р }

    $поток = New-Object System.IO.MemoryStream
    $бв = New-Object System.IO.BinaryWriter($поток)
    $бв.Write([UInt16]0)
    $бв.Write([UInt16]1)
    $бв.Write([UInt16]$размеры.Count)

    $данные = @()
    foreach ($бм in $картинки) {
        $мс = New-Object System.IO.MemoryStream
        $бм.Save($мс, [System.Drawing.Imaging.ImageFormat]::Png)
        $данные += ,$мс.ToArray()
        $мс.Dispose()
        $бм.Dispose()
    }

    $смещение = 6 + 16 * $размеры.Count
    for ($i = 0; $i -lt $размеры.Count; $i++) {
        $р = $размеры[$i]
        $байт = if ($р -ge 256) { 0 } else { $р }
        $бв.Write([Byte]$байт)
        $бв.Write([Byte]$байт)
        $бв.Write([Byte]0)
        $бв.Write([Byte]0)
        $бв.Write([UInt16]1)
        $бв.Write([UInt16]32)
        $бв.Write([UInt32]$данные[$i].Length)
        $бв.Write([UInt32]$смещение)
        $смещение += $данные[$i].Length
    }
    foreach ($д in $данные) { $бв.Write($д) }

    [System.IO.File]::WriteAllBytes($путь, $поток.ToArray())
    $бв.Dispose()
    $поток.Dispose()

    Write-Host "Иконка создана: $путь"
}

Write-Ico -путь (Join-Path $PSScriptRoot "icon.ico") -размеры @(16, 24, 32, 48, 64, 128, 256)
Write-Ico -путь (Join-Path $PSScriptRoot "icon-small.ico") -размеры @(16, 32, 48, 64)
