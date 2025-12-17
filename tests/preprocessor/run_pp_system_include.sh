#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/system_include_bool.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" >"$TMP_OUTPUT" 2>&1

if ! grep -q "ret i32 99" "$TMP_OUTPUT"; then
  echo "expected system include (shadow.h) value 99 in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor system include (shadow.h) test passed."
