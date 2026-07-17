#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

compiler_status=0
"$BIN" tests/parser/compound_literal_nested_braces.c > "$TMP_OUTPUT" 2>&1 || compiler_status=$?
if [ "$compiler_status" -ne 0 ]; then
  echo "Expected compiler exit 0 for nested compound literal braces; got $compiler_status" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

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
