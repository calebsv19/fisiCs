#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/designated_init.c > "$TMP_OUTPUT"

if ! grep -Fq ".x = NUMBER_LITERAL 10" "$TMP_OUTPUT"; then
  echo "Expected .x designated initializer not found" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq ".y = NUMBER_LITERAL 20" "$TMP_OUTPUT"; then
  echo "Expected .y designated initializer not found" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "designated_init test passed."
