#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/syntax/semantic_tag_conflicts.c >"$TMP_OUTPUT" 2>&1 || true

grep -F "Conflicting definition of struct 'Node'" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected struct conflict diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Conflicting definition of union 'Payload'" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected union conflict diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

grep -F "Conflicting definition of enum 'Shade'" "$TMP_OUTPUT" >/dev/null \
  || { echo "Expected enum conflict diagnostic" >&2; cat "$TMP_OUTPUT" >&2; exit 1; }

echo "semantic_tag_conflicts test passed."
