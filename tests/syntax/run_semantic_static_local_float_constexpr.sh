#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

if ! "$BIN" -std=c11 tests/syntax/semantic_static_local_float_constexpr.c >"$TMP_OUTPUT" 2>&1; then
  echo "semantic_static_local_float_constexpr failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "static local initializer is not a constant expression" "$TMP_OUTPUT"; then
  echo "Unexpected static-local const-init diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "Error:" "$TMP_OUTPUT"; then
  echo "semantic_static_local_float_constexpr produced diagnostics unexpectedly" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_static_local_float_constexpr test passed."
