#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_bool.c > "$TMP_OUTPUT"

if grep -Fq "Undeclared identifier" "$TMP_OUTPUT"; then
  echo "Boolean literals not recognized as builtins" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_bool test passed."
