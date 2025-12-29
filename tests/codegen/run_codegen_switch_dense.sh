#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_switch_dense.c >"$TMP_OUTPUT" 2>&1

if ! grep -q "switch i32" "$TMP_OUTPUT"; then
  echo "Expected LLVM switch instruction for dense cases" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_switch_dense test passed."
