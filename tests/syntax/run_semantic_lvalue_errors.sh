#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_lvalue_errors.c > "$TMP_OUTPUT" 2>&1 || true

if ! grep -Fq "Left operand of '=' must be a modifiable lvalue" "$TMP_OUTPUT"; then
  echo "Expected assignment lvalue diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Operator '++' requires modifiable lvalue operand" "$TMP_OUTPUT"; then
  echo "Expected increment diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_lvalue_errors test passed."
