#!/usr/bin/env bash
set -euo pipefail

BIN_DIR=$(dirname "$0")
ROOT_DIR=$(cd "$BIN_DIR/../.." && pwd)

SHARED_ROOT="${FISICS_TEST_SHARED_ROOT:-$ROOT_DIR/third_party/codework_shared}"

SRC_EXPR="$ROOT_DIR/tests/preprocessor/pp_expr_runner.c"
OUT_BIN=$(mktemp)
trap 'rm -f "$OUT_BIN"' EXIT

cc -Wall -Wextra -std=c99 \
  -I"$ROOT_DIR/src" \
  -I"$ROOT_DIR/src/Lexer" \
  -I"$ROOT_DIR/src/Preprocessor" \
  -I"$SHARED_ROOT/core/core_base/include" \
  -I"$SHARED_ROOT/core/core_io/include" \
  -I"$SHARED_ROOT/core/core_data/include" \
  -I"$SHARED_ROOT/core/core_pack/include" \
  "$SRC_EXPR" \
  "$ROOT_DIR/src/Preprocessor/pp_expr.c" \
  "$ROOT_DIR/src/Preprocessor/macro_table.c" \
  "$ROOT_DIR/src/Lexer/lexer.c" \
  "$ROOT_DIR/src/Lexer/lexer_token_handlers.c" \
  "$ROOT_DIR/src/Lexer/token_buffer.c" \
  "$ROOT_DIR/src/Lexer/keyword_lookup.c" \
  -o "$OUT_BIN"

"$OUT_BIN"
echo "preprocessor expression evaluator test passed."
