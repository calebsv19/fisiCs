#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/compound_literal_pointer_decay.c > "$TMP_OUTPUT" 2>&1

if ! grep -Fq "compound.literal.decay" "$TMP_OUTPUT"; then
  echo "Expected compound literal decay to pointer in IR" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "compound_literal_pointer_decay test passed."
