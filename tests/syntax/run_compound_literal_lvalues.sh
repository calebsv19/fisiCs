#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/compound_literal_lvalues.c > "$TMP_OUTPUT" 2>&1 || true

if grep -Fq "Left operand of '=' must be a modifiable lvalue" "$TMP_OUTPUT"; then
  echo "Expected block-scope compound literal assignment to succeed but saw lvalue error" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "compound_literal_lvalues test passed."
