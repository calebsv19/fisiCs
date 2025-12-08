#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/cast_grouped.c > "$TMP_OUTPUT"

count=$(grep -c "CAST_EXPRESSION" "$TMP_OUTPUT" || true)
if [ "$count" -ne 1 ]; then
  echo "Expected exactly one CAST_EXPRESSION, found $count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "cast_grouped test passed."
