#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/parser/switch_flow.c > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "switch i32" "$TMP_OUTPUT"; then
  echo "Expected switch instruction in IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "switch.end" "$TMP_OUTPUT"; then
  echo "Expected switch.end block for break handling" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "switch.default" "$TMP_OUTPUT"; then
  echo "Expected switch.default block for default case" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "switch_flow test passed."
