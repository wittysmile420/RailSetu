#!/usr/bin/env sh
set -eu
mkdir -p build
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/railsetu_stress 64 10
