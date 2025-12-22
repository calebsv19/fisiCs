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

DISABLE_CODEGEN=1 "$BIN" --dump-ast --trigraphs "$SRC" > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "NUMBER_LITERAL 3" "$TMP_OUTPUT" || ! grep -Fq "NUMBER_LITERAL 4" "$TMP_OUTPUT"; then
  echo "Expected trigraph/digraph sample to expand to literals 3 and 4" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor trigraph/digraph test passed."
