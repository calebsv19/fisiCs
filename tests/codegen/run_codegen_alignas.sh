#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_alignas.c > "$TMP_OUTPUT" 2>&1

if ! grep -q "@g_aligned =.*align 128" "$TMP_OUTPUT"; then
  echo "Expected global align 128 for g_aligned" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "%local = alloca .*align 32" "$TMP_OUTPUT"; then
  echo "Expected local alloca with align 32" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "%a = alloca .*align 32" "$TMP_OUTPUT"; then
  echo "Expected struct alloca with align 32" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_alignas test passed."
