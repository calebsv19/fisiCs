#!/usr/bin/env bash
set -euo pipefail

BIN_DIR=$(dirname "$0")
ROOT_DIR=$(cd "$BIN_DIR/../.." && pwd)

SRC_EXPR="$ROOT_DIR/tests/preprocessor/pp_expr_runner.c"
OUT_BIN=$(mktemp)
trap 'rm -f "$OUT_BIN"' EXIT

# Ensure the generated keyword lookup exists (make clean deletes it).
if [ ! -f "$ROOT_DIR/src/Lexer/keyword_lookup.c" ]; then
  gperf "$ROOT_DIR/src/Lexer/keywords.gperf" > "$ROOT_DIR/src/Lexer/keyword_lookup.c"
fi

cc -Wall -Wextra -std=c99 \
  -Isrc -Isrc/Lexer -Isrc/Preprocessor \
  "$SRC_EXPR" \
  "$ROOT_DIR/src/Preprocessor/pp_expr.c" \
  "$ROOT_DIR/src/Preprocessor/macro_table.c" \
  "$ROOT_DIR/src/Lexer/lexer.c" \
  "$ROOT_DIR/src/Lexer/token_buffer.c" \
  "$ROOT_DIR/src/Lexer/keyword_lookup.c" \
  -o "$OUT_BIN"

"$OUT_BIN"
echo "preprocessor expression evaluator test passed."
