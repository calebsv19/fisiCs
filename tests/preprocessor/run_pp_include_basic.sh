#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_basic.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if ! grep -Fq "123" "$TMP_OUTPUT"; then
  echo "Expected LOCAL_VALUE (123) in AST output from include_basic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "300" "$TMP_OUTPUT"; then
  echo "Expected SYS_VALUE (300) from <pp_sys.h> in AST output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor include basic test passed."
