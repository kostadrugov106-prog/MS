param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [string]$Out = "$env:TEMP\gigatool\shot.png",
    [int]$ClickX = -1,
    [int]$ClickY = -1,
    [int]$CommandId = 0,
    [int]$WaitMs = 500,
    [int]$HoldSeconds = 6,
    [switch]$StopAfter
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

$p = Start-Process -FilePath $Exe -PassThru
"pid: $($p.Id)"

$h = [IntPtr]::Zero
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($sw.ElapsedMilliseconds -lt 10000) {
    $p.Refresh()
    if ($p.HasExited) {
        "process exited at $($sw.ElapsedMilliseconds) ms, code $($p.ExitCode)"
        exit 2
    }
    if ($p.MainWindowHandle -ne [IntPtr]::Zero) {
        $h = $p.MainWindowHandle
        break
    }
    Start-Sleep -Milliseconds 40
}
if ($h -eq [IntPtr]::Zero) { "no window after 10s"; exit 3 }
"window at $($sw.ElapsedMilliseconds) ms: $h"

[WinApi]::SetForegroundWindow($h) | Out-Null
Start-Sleep -Milliseconds 300

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
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()

"file: $Out"
"window: $w x $hh, client: $($cr.Right) x $($cr.Bottom), client origin: $($pt.X), $($pt.Y)"

if ($StopAfter) { Stop-Process -Id $p.Id -Force }
