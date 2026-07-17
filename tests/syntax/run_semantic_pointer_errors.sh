#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

set +e
"$BIN" tests/syntax/semantic_pointer_errors.c > "$TMP_OUTPUT" 2>&1
status=$?
set -e
if [ "$status" -ne 1 ]; then
  echo "Expected compiler exit 1, got $status" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "pointer arithmetic (pointer +/- integer)" "$TMP_OUTPUT" &&
   ! grep -Fq "pointer +/- integer operands" "$TMP_OUTPUT"; then
  echo "Expected diagnostic for pointer arithmetic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "comparable operands" "$TMP_OUTPUT"; then
  echo "Expected diagnostic for invalid pointer comparison" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_pointer_errors test passed."
