$ErrorActionPreference = "Stop"
Set-Location -LiteralPath $PSScriptRoot

$пакет = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Directory -Filter "BrechtSanders.WinLibs.POSIX.MSVCRT_*" | Select-Object -First 1
if (-not $пакет) { throw "Не найден компилятор MinGW MSVCRT." }
$бин = Join-Path $пакет.FullName "mingw64\bin"

Copy-Item -LiteralPath (Join-Path $PSScriptRoot "..\движок\ms.exe") -Destination (Join-Path $PSScriptRoot "engine.exe") -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot "..\центр\ms_center.exe") -Destination (Join-Path $PSScriptRoot "center.exe") -Force

& (Join-Path $бин "windres.exe") "ресурс.rc" -O coff -o "ресурс.o"
if ($LASTEXITCODE -ne 0) { throw "windres завершился с ошибкой" }

& (Join-Path $бин "gcc.exe") "ms_setup.c" "ресурс.o" -o "MS-1.1-Setup.exe" -O2 -s -mwindows -static -static-libgcc -D_WIN32_WINNT=0x0601 -DWINVER=0x0601 -ladvapi32 -lole32 -luuid -lshell32 -lmsimg32 -lgdi32 -luser32
if ($LASTEXITCODE -ne 0) { throw "gcc завершился с ошибкой" }

Remove-Item -LiteralPath "engine.exe","center.exe","ресурс.o" -Force
Write-Host "Готово: MS-1.1-Setup.exe" -ForegroundColor Green
