#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/if_control.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if grep -Fq "drop_" "$TMP_OUTPUT"; then
  echo "Inactive branch tokens leaked into output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "keep_two" "$TMP_OUTPUT"; then
  echo "Active branch tokens missing from output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor #if/#elif/#else test passed."
