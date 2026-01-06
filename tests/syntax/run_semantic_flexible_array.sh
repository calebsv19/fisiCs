#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_flexible_array.c > "$TMP_OUTPUT" 2>&1 || true

if ! grep -Fq "Flexible array member must be the last field in a struct" "$TMP_OUTPUT"; then
  echo "Expected flexible-array position diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Flexible array members are not allowed in unions" "$TMP_OUTPUT"; then
  echo "Expected flexible-array union diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_flexible_array test passed."
