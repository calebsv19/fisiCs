#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_next.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if ! grep -Fq "222" "$TMP_OUTPUT"; then
  echo "Expected NEXT_VALUE from include/ (222) via include_next" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "111" "$TMP_OUTPUT"; then
  echo "Unexpected local NEXT_VALUE (111) leaked through include_next search" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor include_next search-order test passed."
