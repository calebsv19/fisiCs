#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

fail=0

for SRC in tests/preprocessor/macro_recursion_self.c tests/preprocessor/macro_recursion_mutual.c; do
  TMP_OUTPUT=$(mktemp)
  if "$BIN" "$SRC" >"$TMP_OUTPUT" 2>&1; then
    echo "expected recursion failure for $SRC but compilation succeeded" >&2
    cat "$TMP_OUTPUT" >&2
    fail=1
  fi
  rm -f "$TMP_OUTPUT"
done

if [ "$fail" -ne 0 ]; then
  exit 1
fi

echo "preprocessor macro recursion detection tests passed."
