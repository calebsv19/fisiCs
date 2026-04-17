#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

if ! "$BIN" -std=c11 tests/syntax/semantic_static_assert_member_array_size.c >"$TMP_OUTPUT" 2>&1; then
  echo "semantic_static_assert_member_array_size failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "Error:" "$TMP_OUTPUT"; then
  echo "semantic_static_assert_member_array_size produced diagnostics unexpectedly" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_static_assert_member_array_size test passed."
