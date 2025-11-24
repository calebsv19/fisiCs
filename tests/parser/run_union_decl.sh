#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/union_decl.c > "$TMP_OUTPUT"

if ! grep -Fq "TYPE: union Data" "$TMP_OUTPUT"; then
  echo "Expected union variable declaration not found in AST output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "union_decl test passed."
