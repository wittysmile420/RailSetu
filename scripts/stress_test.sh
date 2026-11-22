#!/usr/bin/env sh
set -eu
cmake -S . -B build
cmake --build build
exec ./build/railsetu_stress "${1:-100}" "${2:-20}"
