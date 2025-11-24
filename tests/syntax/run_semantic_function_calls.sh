#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_function_calls.c > "$TMP_OUTPUT" 2>&1 || true

grep -F "Too few arguments in call to 'takes_two' (expected 2, got 1)" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected too-few-arguments diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Too many arguments in call to 'takes_two' (expected 2, got 3)" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected too-many-arguments diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Argument 1 of 'takes_two' has incompatible type" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected argument type diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Argument 1 of 'takes_mutable' discards qualifiers from pointer target" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected qualifier drop diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

echo "semantic_function_calls test passed."
