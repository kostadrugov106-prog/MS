$ErrorActionPreference = "Stop"
Set-Location -LiteralPath $PSScriptRoot

$пакет = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Directory -Filter "BrechtSanders.WinLibs.POSIX.MSVCRT_*" | Select-Object -First 1
if (-not $пакет) { throw "Не найден компилятор MinGW MSVCRT." }
$бин = Join-Path $пакет.FullName "mingw64\bin"

$временная = Join-Path $env:TEMP ("ms-center-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $временная | Out-Null
try {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "ms_center.c") -Destination (Join-Path $временная "ms_center.c")
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "ms_center.manifest") -Destination (Join-Path $временная "ms_center.manifest")
    $rc = Get-ChildItem -LiteralPath $PSScriptRoot -Filter *.rc | Select-Object -First 1
    if (-not $rc) { throw "Не найден файл ресурсов .rc" }
    Copy-Item -LiteralPath $rc.FullName -Destination (Join-Path $временная "resource.rc")
    $иконка = Join-Path $PSScriptRoot "..\движок\icon-small.ico"
    if (-not (Test-Path -LiteralPath $иконка)) { throw "Не найден icon-small.ico" }
    Copy-Item -LiteralPath $иконка -Destination (Join-Path $временная "icon-small.ico")

    Push-Location $временная
    try {
        & (Join-Path $бин "windres.exe") "resource.rc" -O coff -o "resource.o"
        if ($LASTEXITCODE -ne 0) { throw "windres завершился с ошибкой" }

        & (Join-Path $бин "gcc.exe") "ms_center.c" "resource.o" -o "ms_center.exe" -O2 -s -mwindows -static -static-libgcc -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -lcomctl32 -lshell32 -lole32 -luuid -ladvapi32 -lwinhttp -lole32 -lgdi32 -luser32 -luxtheme
        if ($LASTEXITCODE -ne 0) { throw "gcc завершился с ошибкой" }
    } finally {
        Pop-Location
    }

    Copy-Item -LiteralPath (Join-Path $временная "ms_center.exe") -Destination (Join-Path $PSScriptRoot "ms_center.exe") -Force
    $итог = Get-Item -LiteralPath (Join-Path $PSScriptRoot "ms_center.exe")
    Write-Host ("Готово: ms_center.exe, {0:N0} байт" -f $итог.Length) -ForegroundColor Green
    Write-Host ("Описание: " + $итог.VersionInfo.FileDescription)
} finally {
    Remove-Item -LiteralPath $временная -Recurse -Force -ErrorAction SilentlyContinue
}
