#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
SRC="tests/preprocessor/nested_macro.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if grep -E "(WRAP|INNER|BASE)" "$TMP_OUTPUT" >/dev/null; then
  echo "Nested macro names should be fully expanded" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "NUMBER_LITERAL 3" "$TMP_OUTPUT"; then
  echo "Expected nested macro expansion to inline BASE into expression" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "BINARY EXPRESSION '*'" "$TMP_OUTPUT"; then
  echo "Expected nested macro expansion to produce multiplied inner results" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor nested macro expansion test passed."
