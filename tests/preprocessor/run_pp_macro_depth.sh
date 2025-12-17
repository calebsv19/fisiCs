#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/macro_recursion_depth.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

# With default limit this should succeed.
"${BIN}" "$SRC" >"$TMP_OUTPUT"

# Now force a tiny expansion limit to trip the guard and expect failure.
if FISICS_MACRO_EXP_LIMIT=8 "${BIN}" "$SRC" >"$TMP_OUTPUT" 2>&1; then
  echo "expected depth guard failure when limit exceeded" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor macro expansion depth test passed."
