#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/comma_decl_init.c > "$TMP_OUTPUT"

grep -Fq "IDENTIFIER a" "$TMP_OUTPUT" || { echo "Expected identifier a in AST" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }
grep -Fq "IDENTIFIER b" "$TMP_OUTPUT" || { echo "Expected identifier b in AST" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

if grep -Fq "Undeclared identifier" "$TMP_OUTPUT"; then
  echo "Unexpected undeclared identifier diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "COMMA_EXPRESSION" "$TMP_OUTPUT"; then
  echo "Unexpected comma expression in declarator initializer" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "comma_decl_init test passed."
