#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_struct_definition_only.c > "$TMP_OUTPUT" 2>&1 || true

if grep -Fq "Error" "$TMP_OUTPUT"; then
  echo "Unexpected errors for bare struct definition test" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_struct_definition_only test passed."
