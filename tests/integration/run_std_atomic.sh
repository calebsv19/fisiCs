#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="${2:-tests/syntax/semantic_atomic_qualifier.c}"
OUT="${3:-/tmp/fisics-std-atomic.o}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT" "$OUT"' EXIT

if ! "$BIN" -std=c11 -c "$SRC" -o "$OUT" >"$TMP_OUTPUT" 2>&1; then
  echo "-std=c11 atomic compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$OUT" ]; then
  echo "Expected object file missing: $OUT" >&2
  exit 1
fi

if grep -Fq "_Atomic is not supported yet" "$TMP_OUTPUT"; then
  echo "Unexpected _Atomic unsupported diagnostic in -std=c11 mode" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "std_atomic integration test passed: $OUT"
