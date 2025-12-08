#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/typedef_chain.c > "$TMP_OUTPUT"

if ! grep -Fq "Alias:     IDENTIFIER ULong" "$TMP_OUTPUT"; then
  echo "Expected typedef alias ULong not found in AST" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "typedef_chain test passed."
