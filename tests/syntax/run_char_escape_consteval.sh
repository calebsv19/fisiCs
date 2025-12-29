#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/char_escape_consteval.c >"$TMP_OUTPUT" 2>&1 || true

if ! grep -q "Character literal value is out of range" "$TMP_OUTPUT"; then
  echo "Expected out-of-range wide char diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "char_escape_consteval test passed."
