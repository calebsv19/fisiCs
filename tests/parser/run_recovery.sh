#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SPEC_FILTER="parser/recovery.c" tests/spec/run_ast_golden.sh "$BIN"
echo "parser recovery test passed."
