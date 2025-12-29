#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_switch_pointer_selector.c >"$TMP_OUTPUT" 2>&1

if grep -q "switch i" "$TMP_OUTPUT"; then
  echo "Pointer switch should not lower to LLVM switch instruction" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "ptrtoint" "$TMP_OUTPUT"; then
  echo "Pointer switch should compare via ptrtoint chain" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_switch_pointer_selector test passed."
