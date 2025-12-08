#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_search_order.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if ! grep -Fq "7" "$TMP_OUTPUT"; then
  echo "Expected SHADOW_VAL from local header (7) in output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "99" "$TMP_OUTPUT"; then
  echo "System include version of shadow.h was used instead of local copy" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor include search-order test passed."
