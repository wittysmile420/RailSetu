#!/usr/bin/env sh
set -eu
if [ ! -x "./build/railsetu_app" ]; then
  ./scripts/build.sh
fi
exec ./build/railsetu_app "$@"
