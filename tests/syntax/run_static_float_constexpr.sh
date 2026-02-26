#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/static_float_constexpr.c >"$TMP_OUTPUT" 2>&1 || true

if grep -Fq "Initializer for static variable" "$TMP_OUTPUT"; then
  echo "Unexpected static constant-expression diagnostic for float/double literal init" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "static_float_constexpr test passed."
