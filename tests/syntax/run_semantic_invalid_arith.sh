#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_invalid_arith.c > "$TMP_OUTPUT" 2>&1 || true

if ! grep -Fq "Operator '+'" "$TMP_OUTPUT"; then
  echo "Expected diagnostic for struct + int" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Operator '&' requires integer operands" "$TMP_OUTPUT"; then
  echo "Expected diagnostic for bitwise on non-integer" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_invalid_arith test passed."
