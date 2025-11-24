#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

# Capture both stdout (parser/semantic dumps) and stderr (LLVM IR) so we can
# filter down to the module text only.
"$BIN" tests/codegen/codegen_pointer_deref.c > "$TMP_OUTPUT" 2>&1

if ! awk '
  /^; ModuleID/ { in_ir = 1 }
  /^DEBUG:/ { in_ir = 0 }
  in_ir && index($0, "load i32, ptr %") { found = 1; exit 0 }
  END { exit found ? 0 : 1 }
' "$TMP_OUTPUT"; then
  echo "Expected pointer dereference load in LLVM IR output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_pointer_deref test passed."
