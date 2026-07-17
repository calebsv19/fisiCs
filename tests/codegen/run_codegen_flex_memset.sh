#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_flex_memset.c > "$TMP_OUTPUT" 2>&1

if ! grep -q "%Agg = type { \[0 x <32 x i8>\], i32, \[28 x i8\] }" "$TMP_OUTPUT"; then
  echo "Expected aggregate type to carry align-32 anchor and tail padding" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "%a = alloca %Agg, align 32" "$TMP_OUTPUT"; then
  echo "Expected over-aligned aggregate alloca with align 32" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "memset.*ptr align 32 %a.*i64 32" "$TMP_OUTPUT"; then
  echo "Expected 32-byte aggregate zero-init on the aligned base object" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_flex_memset test passed."
