#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_flow.c >"$TMP_OUTPUT" 2>&1 || true

grep -F "Control reaches end of non-void function 'missing_return'" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected missing-return diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Unreachable statement" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected unreachable diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Switch case may fall through" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected switch fallthrough diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

echo "semantic_flow test passed."
