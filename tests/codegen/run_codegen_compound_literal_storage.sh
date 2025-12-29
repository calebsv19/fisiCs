#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_compound_literal_storage.c > "$TMP_OUTPUT" 2>&1

if ! grep -q "compound.tmp" "$TMP_OUTPUT"; then
  echo "Expected compound literal stack slot emission" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -E -q "alloca %P|alloca %struct\\.P" "$TMP_OUTPUT"; then
  echo "Expected compound literal alloca for automatic lifetime" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_compound_literal_storage test passed."
