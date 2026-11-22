$ErrorActionPreference = "Stop"
$buildDir = "build"
$compiler = (Get-Command g++ -ErrorAction SilentlyContinue)
if (-not $compiler) {
    throw "g++ was not found. Install/activate MSYS2 UCRT64 or use a Visual Studio Developer PowerShell."
}
Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
cmake -S . -B $buildDir -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build $buildDir --parallel
ctest --test-dir $buildDir --output-on-failure
& "$buildDir\railsetu_stress.exe" 64 10
