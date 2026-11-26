#!/usr/bin/env sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"
echo "RailSetu Web is starting at http://localhost:4173"
exec python3 -m http.server 4173 --bind 127.0.0.1 --directory "$ROOT/web"
