#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_restrict_alias.c > "$TMP_OUTPUT"

if ! grep -Fq "Restrict parameters may alias" "$TMP_OUTPUT"; then
  echo "Expected restrict alias warning not found" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_restrict_alias test passed."
