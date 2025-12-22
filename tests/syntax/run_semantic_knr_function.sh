#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_knr_function.c > "$TMP_OUTPUT"

if grep -Fq "Error" "$TMP_OUTPUT"; then
  echo "Semantic analysis reported errors unexpectedly for K&R function" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_knr_function test passed."
