#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/control_flow.c > "$TMP_OUTPUT"

grep -Fq "IF_STATEMENT" "$TMP_OUTPUT" || { echo "Missing IF_STATEMENT in AST" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }
grep -Fq "ELSE_STATEMENT" "$TMP_OUTPUT" || { echo "Missing ELSE_STATEMENT in AST" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }
grep -Fq "WHILE_LOOP" "$TMP_OUTPUT" || { echo "Missing WHILE_LOOP in AST" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

echo "control_flow test passed."
