param(
    [string]$ProcessName = "MS-1.0-Setup",
    [string]$OutFile = "$env:TEMP\ms_shot.png",
    [int]$ClickX = -1,
    [int]$ClickY = -1,
    [int]$CommandId = 0,
    [int]$WaitMs = 1200
)

Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class WinApi {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
"@

$proc = $null
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 60; $i++) {
    $proc = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($proc) {
        $proc.Refresh()
        if ($proc.MainWindowHandle -ne [IntPtr]::Zero) {
            $h = $proc.MainWindowHandle
            break
        }
    }
    Start-Sleep -Milliseconds 150
}
if ($h -eq [IntPtr]::Zero) { throw "Window not found for process $ProcessName" }

[WinApi]::SetForegroundWindow($h) | Out-Null

if ($CommandId -gt 0) {
    [WinApi]::PostMessage($h, 0x0111, [IntPtr]$CommandId, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Milliseconds $WaitMs
}

if ($ClickX -ge 0 -and $ClickY -ge 0) {
    $lp = [IntPtr](($ClickY -shl 16) -bor ($ClickX -band 0xFFFF))
    [WinApi]::PostMessage($h, 0x0201, [IntPtr]1, $lp) | Out-Null
    Start-Sleep -Milliseconds 80
    [WinApi]::PostMessage($h, 0x0202, [IntPtr]0, $lp) | Out-Null
    Start-Sleep -Milliseconds $WaitMs
}

$wr = New-Object WinApi+RECT
[WinApi]::GetWindowRect($h, [ref]$wr) | Out-Null
$cr = New-Object WinApi+RECT
[WinApi]::GetClientRect($h, [ref]$cr) | Out-Null
$pt = New-Object WinApi+POINT
$pt.X = 0; $pt.Y = 0
[WinApi]::ClientToScreen($h, [ref]$pt) | Out-Null

$w = $wr.Right - $wr.Left
$hh = $wr.Bottom - $wr.Top
$bmp = New-Object System.Drawing.Bitmap($w, $hh)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
[WinApi]::PrintWindow($h, $dc, 2) | Out-Null
$g.ReleaseHdc($dc)
$g.Dispose()
$bmp.Save($OutFile, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

"file: $OutFile"
"window: $w x $hh, client: $($cr.Right) x $($cr.Bottom), client origin: $($pt.X), $($pt.Y)"
"process: $($proc.Id)"
