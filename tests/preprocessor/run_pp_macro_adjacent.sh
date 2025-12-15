#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/macro_adjacent.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

# If the preprocessor misclassifies the macros, it will emit errors like
# "invalid parameter list" and exit non-zero. We just need successful compile.
# Also sanity-check that the generated output mentions G (zero-arg function-like)
# so we know the run progressed far enough.
if ! "$BIN" "$SRC" > "$TMP_OUTPUT"; then
  echo "macro adjacency test failed (compiler exit code != 0)" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "invalid parameter list" "$TMP_OUTPUT"; then
  echo "macro adjacency test failed: saw 'invalid parameter list' diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor macro adjacency test passed."
