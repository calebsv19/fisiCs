#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
SRC="tests/preprocessor/variadic_macro.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

# The variadic macro expands to ((1)+(2)+(3)), so the AST should mention 1, 2, and 3.
for n in 1 2 3; do
  if ! grep -Fq "$n" "$TMP_OUTPUT"; then
    echo "Expected number $n in AST output from variadic macro expansion" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
done

echo "preprocessor variadic macro test passed."
