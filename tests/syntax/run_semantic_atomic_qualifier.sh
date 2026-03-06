#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_atomic_qualifier.c > "$TMP_OUTPUT" 2>&1 || true

if ! grep -Fq "_Atomic is not supported yet" "$TMP_OUTPUT"; then
  echo "Expected explicit unsupported diagnostic for _Atomic qualifiers/specifiers" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_atomic_qualifier unsupported-policy test passed."
