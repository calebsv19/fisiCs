#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

ENABLE_GNU_STATEMENT_EXPRESSIONS=0 "$BIN" tests/parser/gnu_statement_expr.c > "$TMP_OUTPUT" 2>&1 || true

if ! grep -Fq "GNU statement expressions are disabled" "$TMP_OUTPUT"; then
  echo "Expected diagnostic when statement expressions disabled" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "statement_expr (disabled) test passed."
