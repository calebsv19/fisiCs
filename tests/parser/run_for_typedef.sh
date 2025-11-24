#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/for_typedef.c > "$TMP_OUTPUT"

grep -Fq "FOR_LOOP" "$TMP_OUTPUT" || { echo "Expected FOR_LOOP in AST" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }
grep -Fq "TYPE: I" "$TMP_OUTPUT" || { echo "Expected typedef initializer in for-loop" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

echo "for_typedef test passed."
