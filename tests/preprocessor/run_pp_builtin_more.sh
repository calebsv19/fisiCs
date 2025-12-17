#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/builtin_more.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" >"$TMP_OUTPUT"

# Sanity: remapped filename should appear and line remap should be honored.
if ! grep -q "foo.c" "$TMP_OUTPUT"; then
  echo "expected __FILE__ to reflect #line remap (foo.c)" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi
if ! grep -q "NUMBER_LITERAL 104" "$TMP_OUTPUT"; then
  echo "expected __LINE__ remap to produce 104" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor extended built-ins test passed."
