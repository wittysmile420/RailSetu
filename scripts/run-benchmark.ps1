$ErrorActionPreference = "Stop"
$buildDir = "build"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
cmake -S . -B $buildDir
cmake --build $buildDir --config Release
$stressExe = Join-Path $buildDir "railsetu_stress.exe"
$releaseExe = Join-Path $buildDir "Release\railsetu_stress.exe"
if (Test-Path $stressExe) {
    & $stressExe 64 10
} elseif (Test-Path $releaseExe) {
    & $releaseExe 64 10
} else {
    throw "railsetu_stress.exe was not found in build\ or build\Release\"
}
