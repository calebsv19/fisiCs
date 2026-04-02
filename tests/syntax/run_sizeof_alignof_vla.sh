#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/sizeof_alignof_vla.c >"$TMP_OUTPUT" 2>&1 || true

if ! grep -q "alignof requires type-name operand" "$TMP_OUTPUT"; then
  echo "Expected strict _Alignof(type-name) diagnostic for _Alignof(vla) operand" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "sizeof_alignof_vla test passed."
