#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
ROOT="tests/preprocessor/diag_header_path"
SRC="$ROOT/main.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

if "$BIN" "$SRC" > "$TMP_OUTPUT" 2>&1; then
  echo "diagnostic header path test failed: expected preprocessing error but got success" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

# Expect the diagnostic to point to bad.h, not main.c
if ! grep -Fq "bad.h" "$TMP_OUTPUT"; then
  echo "diagnostic header path test failed: diagnostic did not reference bad.h" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "main.c" "$TMP_OUTPUT"; then
  echo "diagnostic header path test failed: diagnostic incorrectly referenced main.c" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor diagnostic header path test passed."
