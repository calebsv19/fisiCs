#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_shift.c >"$TMP_OUTPUT" 2>&1

if ! grep -q "lshr i32" "$TMP_OUTPUT"; then
  echo "Expected logical right shift (lshr) in IR" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_shift test passed."
