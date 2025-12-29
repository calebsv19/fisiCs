#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

# Allow skipping const-eval of heavy system globals for this stress case.
FISICS_NO_CONST_GLOBALS=1 "$BIN" tests/codegen/codegen_flex_lvalue.c > "$TMP_OUTPUT" 2>&1

# Ensure we emitted IR (no crash) and the return path is present.
if ! grep -q "ret i32" "$TMP_OUTPUT"; then
  echo "Expected IR with integer return; got:" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_flex_lvalue test passed."
