#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_pointer_diff_long.c > "$TMP_OUTPUT" 2>&1

if ! awk '
  /^; ModuleID/ { in_ir = 1 }
  /^DEBUG:/ { in_ir = 0 }
  in_ir && index($0, "ptr.diff") { found = 1; exit 0 }
  END { exit found ? 0 : 1 }
' "$TMP_OUTPUT"; then
  echo "Expected ptr.diff sequence in LLVM IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_pointer_diff_long test passed."
