#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="${2:-tests/parser/union_decl.c}"
OUT="${3:-/tmp/mycc-integration.o}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

trap 'rm -f "$OUT"' EXIT

"$BIN" -c "$SRC" -o "$OUT"

if [ ! -f "$OUT" ]; then
  echo "Expected object file missing: $OUT" >&2
  exit 1
fi

echo "compile_only integration test passed: $OUT"
