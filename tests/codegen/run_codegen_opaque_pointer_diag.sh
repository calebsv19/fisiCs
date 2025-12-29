#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

# Goal: the compiler should not crash when handling a pointer to an incomplete
# type; success (or a clean diagnostic) is acceptable.
if "$BIN" tests/codegen/codegen_opaque_pointer_diag.c >"$TMP_OUTPUT" 2>&1; then
  echo "codegen_opaque_pointer_diag test passed (no crash)."
  exit 0
fi

# If it failed, ensure the failure is a clean diagnostic, not a crash.
if grep -qi "opaque pointer" "$TMP_OUTPUT"; then
  echo "codegen_opaque_pointer_diag test passed (opaque-pointer diagnostic emitted)."
  exit 0
fi

echo "codegen_opaque_pointer_diag test failed:"
cat "$TMP_OUTPUT" >&2
exit 1
