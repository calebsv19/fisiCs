#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_ternary_merge.c >"$TMP_OUTPUT" 2>&1

# Expect select/phi of type ptr (void*).
if ! grep -q "select i1" "$TMP_OUTPUT" && ! grep -q "phi ptr" "$TMP_OUTPUT"; then
  echo "Expected pointer merge in IR (select/phi ptr)" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_ternary_merge test passed."
