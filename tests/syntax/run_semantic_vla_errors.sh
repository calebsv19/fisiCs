#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_vla_errors.c > "$TMP_OUTPUT" 2>&1 || true

if ! grep -Fq "Variable-length array 'file_bad' is not allowed" "$TMP_OUTPUT"; then
  echo "Expected file-scope VLA diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Variable-length array 'static_bad' is not allowed" "$TMP_OUTPUT"; then
  echo "Expected static-scope VLA diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_vla_errors test passed."
