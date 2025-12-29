#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_bitfield.c > "$TMP_OUTPUT" 2>&1

if ! awk '
  /^; ModuleID/ { in_ir = 1 }
  /^DEBUG:/ { in_ir = 0 }
  in_ir && /bf\.mask/ { found = 1 }
  in_ir && /bf\.shl/ { shl = 1 }
  END { exit (found && shl) ? 0 : 1 }
' "$TMP_OUTPUT"; then
  echo "Expected bitfield mask/shift in LLVM IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_bitfield test passed."
