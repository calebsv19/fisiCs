#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/preprocessor/pp_recursive_guard.c >"$TMP_OUTPUT" 2>&1 || {
  echo "recursive include guard fixture failed to compile" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
}

if rg -q "detected recursive include" "$TMP_OUTPUT"; then
  echo "unexpected recursive include diagnostic for guarded cycle" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor recursive-guard include test passed."
