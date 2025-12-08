#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/initializer_expr.c > "$TMP_OUTPUT"

if ! grep -Fq "CAST_EXPRESSION" "$TMP_OUTPUT"; then
  echo "Expected cast expression not printed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "SIZEOF" "$TMP_OUTPUT"; then
  echo "Expected sizeof expression not printed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "initializer_expr test passed."
