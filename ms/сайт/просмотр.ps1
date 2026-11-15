param(
    [int]$Порт = 8765
)

$ErrorActionPreference = "Stop"
$корень = $PSScriptRoot

$слушатель = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, $Порт)
$слушатель.Start()
Write-Host "Сайт MS: http://localhost:$Порт/  (Ctrl+C — остановить)"

function Тип-Содержимого([string]$файл) {
    switch ([IO.Path]::GetExtension($файл).ToLowerInvariant()) {
        ".html" { "text/html; charset=utf-8" }
        ".css"  { "text/css; charset=utf-8" }
        ".js"   { "application/javascript; charset=utf-8" }
        ".svg"  { "image/svg+xml" }
        ".png"  { "image/png" }
        ".ico"  { "image/x-icon" }
        ".xml"  { "application/xml; charset=utf-8" }
        ".txt"  { "text/plain; charset=utf-8" }
        default { "application/octet-stream" }
    }
}

try {
    while ($true) {
        $клиент = $слушатель.AcceptTcpClient()
        try {
            $поток = $клиент.GetStream()
            $читатель = [IO.StreamReader]::new($поток, [Text.Encoding]::ASCII)
            $строка = $читатель.ReadLine()
            if (-not $строка) { continue }
            while (($h = $читатель.ReadLine()) -ne $null -and $h -ne "") { }

            $части = $строка.Split(' ')
            $метод = $части[0]
            $адрес = $части[1]
            if ($адрес.Contains("?")) { $адрес = $адрес.Substring(0, $адрес.IndexOf("?")) }

            $путь = [Uri]::UnescapeDataString($адрес)
            $относительный = $путь.TrimStart("/")
            if ($относительный -eq "") { $относительный = "index.html" }
            if ($относительный.Contains("..")) { throw "Запрещённый путь" }

            $файл = Join-Path $корень ($относительный -replace "/", "\")
            if (-not (Test-Path -LiteralPath $файл -PathType Leaf)) {
                if (Test-Path -LiteralPath ($файл + ".html") -PathType Leaf) {
                    $файл = $файл + ".html"
                } elseif (Test-Path -LiteralPath $файл -PathType Container) {
                    $вложенный = Join-Path $файл "index.html"
                    if (Test-Path -LiteralPath $вложенный -PathType Leaf) { $файл = $вложенный }
                }
            }

            $код = 200
            if (Test-Path -LiteralPath $файл -PathType Leaf) {
                $байты = [IO.File]::ReadAllBytes($файл)
                $тип = Тип-Содержимого $файл
            } else {
                $код = 404
                $байты = [IO.File]::ReadAllBytes((Join-Path $корень "404.html"))
                $тип = "text/html; charset=utf-8"
            }

            $состояние = if ($код -eq 200) { "OK" } else { "Not Found" }
            $заголовки = "HTTP/1.1 $код $состояние`r`n" +
                         "Content-Type: $тип`r`n" +
                         "Content-Length: $($байты.Length)`r`n" +
                         "Cache-Control: no-store`r`n" +
                         "Connection: close`r`n`r`n"
            $байтыЗаголовков = [Text.Encoding]::ASCII.GetBytes($заголовки)
            $поток.Write($байтыЗаголовков, 0, $байтыЗаголовков.Length)
            if ($метод -ne "HEAD") { $поток.Write($байты, 0, $байты.Length) }
            Write-Host "$метод $путь -> $код"
        } catch {
            # неудачный запрос — просто закрываем соединение
        } finally {
            $клиент.Close()
        }
    }
} finally {
    $слушатель.Stop()
}
