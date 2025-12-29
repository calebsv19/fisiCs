#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_callconv_declspec.c >"$TMP_OUTPUT" 2>&1

if ! grep -q "x86_stdcallcc" "$TMP_OUTPUT"; then
  # Allow a warning-only path on targets that ignore the callconv.
  if ! grep -Eq "Calling convention attribute may be ignored|__stdcall ignored" "$TMP_OUTPUT"; then
    echo "Expected stdcall calling convention (or warning about ignoring it)" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
fi

if ! grep -Eq "dllstorageclass\\(export\\)|dllexport" "$TMP_OUTPUT"; then
  if ! grep -q "__declspec(dllexport) ignored" "$TMP_OUTPUT"; then
    echo "Expected dllexport storage class in IR or a diagnostic about ignoring it" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
fi

echo "codegen_callconv_declspec test passed."
