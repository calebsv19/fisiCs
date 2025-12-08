#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_basic.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_DEFAULT=$(mktemp)
TMP_PRESERVE=$(mktemp)
trap 'rm -f "$TMP_DEFAULT" "$TMP_PRESERVE"' EXIT

"$BIN" "$SRC" > "$TMP_DEFAULT"
"$BIN" --preserve-pp "$SRC" > "$TMP_PRESERVE"

if grep -Fq "INCLUDE_DIRECTIVE" "$TMP_DEFAULT"; then
  echo "Unexpected INCLUDE_DIRECTIVE in non-preserve mode" >&2
  exit 1
fi

if ! grep -Fq "INCLUDE_DIRECTIVE" "$TMP_PRESERVE"; then
  echo "Missing INCLUDE_DIRECTIVE in preserve-pp mode" >&2
  cat "$TMP_PRESERVE" >&2
  exit 1
fi

echo "preprocessor preservation toggle test passed."
