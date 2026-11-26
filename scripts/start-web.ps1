$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$web = Join-Path $root "web"
$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    $python = Get-Command py -ErrorAction SilentlyContinue
}
if (-not $python) {
    throw "Python 3 was not found. Install Python 3 to run the local web server."
}
Write-Host "RailSetu Web is starting at http://localhost:4173"
Write-Host "Press Ctrl+C to stop the local server."
if ($python.Name -eq "py.exe") {
    & $python.Source -3 -m http.server 4173 --bind 127.0.0.1 --directory $web
} else {
    & $python.Source -m http.server 4173 --bind 127.0.0.1 --directory $web
}
