#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

set +e
"$BIN" tests/syntax/semantic_pointer_qualifier.c > "$TMP_OUTPUT" 2>&1
status=$?
set -e
if [ "$status" -ne 1 ]; then
  echo "Expected compiler exit 1, got $status" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Assignment discards qualifiers from pointer target" "$TMP_OUTPUT"; then
  echo "Expected qualifier discard diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "semantic_pointer_qualifier test passed."
