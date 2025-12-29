#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/case_non_ice.c >"$TMP_OUTPUT" 2>&1 || true

if ! grep -q "Case label is not an integer constant expression" "$TMP_OUTPUT"; then
  echo "Expected non-ICE case diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "case_non_ice test passed."
