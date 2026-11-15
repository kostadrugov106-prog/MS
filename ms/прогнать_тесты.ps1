param(
    [string]$Движок = (Join-Path $PSScriptRoot "движок\ms.exe")
)

$ErrorActionPreference = "Stop"
try { [Console]::OutputEncoding = [System.Text.Encoding]::UTF8 } catch { }

if (-not (Test-Path -LiteralPath $Движок)) {
    throw "Не найден движок: $Движок. Сначала запустите .\собрать_движок.ps1"
}

$тесты = Join-Path $PSScriptRoot "тесты"
$файлы = @(Get-ChildItem -LiteralPath $тесты -Filter *.ms | Sort-Object Name)
if ($файлы.Count -eq 0) { throw "В папке тесты нет файлов .ms" }

$провалов = 0
Push-Location $тесты
try {
    foreach ($ф in $файлы) {
        $вывод = & $Движок $ф.FullName 2>&1 | Out-String
        $код = $LASTEXITCODE
        if ($код -eq 0) {
            Write-Host ("[ OK ] " + $ф.Name) -ForegroundColor Green
        } else {
            $провалов++
            Write-Host ("[ОШИБКА] " + $ф.Name + " — код " + $код) -ForegroundColor Red
            if ($вывод.Trim()) { Write-Host $вывод }
        }
    }
} finally {
    Pop-Location
}

Write-Host ""
if ($провалов -eq 0) {
    Write-Host ("Все тесты прошли: " + $файлы.Count) -ForegroundColor Green
    exit 0
}
Write-Host ("Провалено: " + $провалов + " из " + $файлы.Count) -ForegroundColor Red
exit 1
