#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_stringize.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

compiler_status=0
"$BIN" "$SRC" > "$TMP_OUTPUT" 2>&1 || compiler_status=$?
if [ "$compiler_status" -ne 0 ]; then
  echo "Expected compiler exit 0 for stringized include; got $compiler_status" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "could not resolve include" "$TMP_OUTPUT"; then
  echo "Unexpected include resolution failure for stringized header" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "Error" "$TMP_OUTPUT"; then
  echo "Unexpected preprocessor error for stringized include" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor include_stringize test passed."
