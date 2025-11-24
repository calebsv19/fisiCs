#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/pointer_arith.c > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "ptr.arith" "$TMP_OUTPUT"; then
  echo "Expected pointer arithmetic GEP in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "ptr.diff" "$TMP_OUTPUT"; then
  echo "Expected pointer difference instruction in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "ptrcmp" "$TMP_OUTPUT"; then
  echo "Expected pointer comparison instruction in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "pointer_arith test passed."
