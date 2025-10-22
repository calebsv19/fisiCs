#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/function_pointer.c > "$TMP_OUTPUT"

if ! grep -Fq "FUNCTION DECLARATION" "$TMP_OUTPUT" && ! grep -Fq "FUNCTION_POINTER" "$TMP_OUTPUT"; then
  if ! grep -Fq "VAR_DECL" "$TMP_OUTPUT" || ! grep -Fq "(*fn)" "$TMP_OUTPUT"; then
    echo "Expected function pointer declaration not present" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
fi

echo "function_pointer test passed."
