#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

if ! "$BIN" tests/syntax/semantic_bitfield_layout.c >"$TMP_OUTPUT" 2>&1; then
  echo "semantic_bitfield_layout failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -q "Error:" "$TMP_OUTPUT"; then
  echo "semantic_bitfield_layout produced diagnostics unexpectedly" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_bitfield_layout test passed."
