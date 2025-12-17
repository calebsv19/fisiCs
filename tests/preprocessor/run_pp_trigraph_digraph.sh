#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/trigraph_digraph.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" --trigraphs "$SRC" > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "ret i32 7" "$TMP_OUTPUT"; then
  echo "Expected trigraph/digraph sample to fold to return 7" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor trigraph/digraph test passed."
