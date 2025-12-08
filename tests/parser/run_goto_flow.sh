#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/goto_flow.c > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "goto.sink" "$TMP_OUTPUT"; then
  echo "Expected goto sink block for control-flow continuation" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "start" "$TMP_OUTPUT"; then
  echo "Expected label 'start' in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "goto_flow test passed."
