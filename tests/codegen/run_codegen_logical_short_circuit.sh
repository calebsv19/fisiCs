#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_logical_short_circuit.c > "$TMP_OUTPUT" 2>&1

if grep -q "<badref>" "$TMP_OUTPUT"; then
  echo "Found <badref> in logical short-circuit codegen" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -q "Error:" "$TMP_OUTPUT"; then
  echo "Unexpected error during logical short-circuit codegen" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_logical_short_circuit test passed."
