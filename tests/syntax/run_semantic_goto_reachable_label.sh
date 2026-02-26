#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_goto_reachable_label.c >"$TMP_OUTPUT" 2>&1

if grep -Fq "Unreachable statement" "$TMP_OUTPUT"; then
  echo "Unexpected unreachable warning for goto-target label path" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_goto_reachable_label test passed."
