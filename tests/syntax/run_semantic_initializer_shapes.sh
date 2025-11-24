#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_initializer_shapes.c >"$TMP_OUTPUT" 2>&1 || true

grep -F "Excess elements in scalar initializer for 'scalar_bad'" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected scalar excess diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Initializer for struct variable 'p_bad' must be brace-enclosed" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected struct initializer diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Too many initializers for array 'arr_too_many' (size 2)" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected array too-many diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Not enough initializers for array 'arr_too_few' (have 1, expected 3)" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected array not-enough diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Array 'arr_designator' designator index 2 is out of bounds" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected designator bounds diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Excess elements in scalar initializer for 'nested'" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected nested initializer diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "String literal for array 'label' is too long" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected string literal diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

echo "semantic_initializer_shapes test passed."
