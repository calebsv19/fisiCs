#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_varargs_call.c >"$TMP_OUTPUT" 2>&1

if ! grep -q "call void @llvm\\.va_start" "$TMP_OUTPUT"; then
  echo "Expected llvm.va_start intrinsic call in IR" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "vararg.fp.promote" "$TMP_OUTPUT"; then
  echo "Expected float argument to be promoted to double for varargs" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "vararg.int.promote" "$TMP_OUTPUT"; then
  echo "Expected default integer promotion for narrow integer varargs" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -E -q "call i32 \\(i32, \\.\\.\\.\\) @vfun" "$TMP_OUTPUT"; then
  echo "Expected call to variadic vfun with promoted arguments" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_varargs_call test passed."
