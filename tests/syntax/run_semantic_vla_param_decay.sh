#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_vla_param_decay.c > "$TMP_OUTPUT" 2>&1 || true

if grep -Fq "Error" "$TMP_OUTPUT"; then
  echo "Unexpected semantic error for VLA param decay" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "Variable-length array" "$TMP_OUTPUT"; then
  echo "Unexpected VLA diagnostic for parameter decay case" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_vla_param_decay test passed."
