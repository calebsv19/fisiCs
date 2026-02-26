#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/compound_literal_nested_braces.c > "$TMP_OUTPUT" 2>&1 || true

if grep -Fq "Unexpected token at start of expression" "$TMP_OUTPUT"; then
  echo "Unexpected parse error for nested compound literal braces" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

grep -Fq "COMPOUND_LITERAL" "$TMP_OUTPUT" || {
  echo "Expected COMPOUND_LITERAL in AST output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
}

echo "compound_literal_nested_braces test passed."
