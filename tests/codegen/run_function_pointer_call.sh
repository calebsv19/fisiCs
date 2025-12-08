#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/function_pointer_call.c > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "call i32" "$TMP_OUTPUT"; then
  echo "Expected function pointer call in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "function_pointer_call test passed."
