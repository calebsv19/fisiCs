#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
ENABLE_GNU_STATEMENT_EXPRESSIONS=1 SPEC_FILTER="parser/gnu_statement_expr.c" tests/spec/run_ast_golden.sh "$BIN"
echo "statement_expr (enabled) test passed."
