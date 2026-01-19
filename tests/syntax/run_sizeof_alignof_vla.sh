#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/sizeof_alignof_vla.c >"$TMP_OUTPUT" 2>&1 || true

if ! grep -q "Semantic analysis: no issues found." "$TMP_OUTPUT"; then
  echo "Expected sizeof/alignof VLA to pass semantic analysis" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "sizeof_alignof_vla test passed."
