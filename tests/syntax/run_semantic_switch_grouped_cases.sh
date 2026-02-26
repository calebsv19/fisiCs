#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_switch_grouped_cases.c >"$TMP_OUTPUT" 2>&1

if grep -Fq "Switch case may fall through" "$TMP_OUTPUT"; then
  echo "Unexpected switch fallthrough warning for grouped cases" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_switch_grouped_cases test passed."
