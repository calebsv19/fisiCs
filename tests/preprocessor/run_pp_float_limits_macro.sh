#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/float_limits_macro.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" >"$TMP_OUTPUT" 2>&1 || true

if grep -Fq "Undeclared identifier" "$TMP_OUTPUT"; then
  echo "float limit macro expansion produced undeclared identifier" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "__FLT_MAX__" "$TMP_OUTPUT" || grep -Fq "__DBL_MAX__" "$TMP_OUTPUT"; then
  echo "expected FLT_MAX/DBL_MAX to expand to numeric literals" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor float limit macro test passed."
