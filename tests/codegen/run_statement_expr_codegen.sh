#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

ENABLE_GNU_STATEMENT_EXPRESSIONS=1 "$BIN" tests/codegen/statement_expr_codegen.c > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "stmt.expr.result" "$TMP_OUTPUT"; then
  echo "Expected statement expression lowering in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "statement_expr codegen test passed."
