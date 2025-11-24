#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/cast_vs_group.c > "$TMP_OUTPUT"

if ! grep -Fq "GROUPED" "$TMP_OUTPUT" || ! grep -Fq "CAST_EXPRESSION" "$TMP_OUTPUT"; then
  echo "Expected grouped expression and cast in AST" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "cast_vs_group test passed."
