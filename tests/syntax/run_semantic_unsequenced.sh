#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_unsequenced.c > "$TMP_OUTPUT"

# Expect at least two warnings for unsequenced access; no fatal errors.
if ! grep -Fq "Unsequenced modification and access" "$TMP_OUTPUT"; then
  echo "Expected unsequenced access warning not found" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_unsequenced test passed."
