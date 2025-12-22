#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/pragma_stdc.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

DISABLE_CODEGEN=1 "$BIN" "$SRC" >"$TMP_OUTPUT" 2>&1

echo "preprocessor pragma STDC stub test passed."
