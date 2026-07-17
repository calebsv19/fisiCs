#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

compiler_status=0
"$BIN" tests/syntax/semantic_undeclared.c > "$TMP_OUTPUT" 2>&1 || compiler_status=$?

if [ "$compiler_status" -ne 1 ]; then
  echo "Expected compiler exit 1 for undeclared identifier, got $compiler_status" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Undeclared identifier" "$TMP_OUTPUT"; then
  echo "Expected undeclared identifier error not reported" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_undeclared test passed (error detected)."
