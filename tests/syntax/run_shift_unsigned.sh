#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/shift_unsigned.c >"$TMP_OUTPUT" 2>&1

if grep -q "Error:" "$TMP_OUTPUT"; then
  echo "shift_unsigned produced diagnostics" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "shift_unsigned test passed."
