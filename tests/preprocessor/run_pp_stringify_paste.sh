#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
SRC="tests/preprocessor/stringify_paste.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if ! grep -Fq "myfunc" "$TMP_OUTPUT"; then
  echo "Expected pasted identifier 'myfunc' in AST output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "\"NAME ( token )\"" "$TMP_OUTPUT"; then
  echo "Expected stringified literal \"NAME ( token )\" in AST output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor stringify/paste test passed."
