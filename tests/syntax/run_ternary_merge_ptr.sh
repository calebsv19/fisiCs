#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/ternary_merge_ptr.c >"$TMP_OUTPUT" 2>&1

if grep -q "Error:" "$TMP_OUTPUT"; then
  echo "ternary_merge_ptr produced diagnostics" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "ternary_merge_ptr test passed."
