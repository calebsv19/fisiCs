#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" tests/codegen/codegen_switch_sparse.c >"$TMP_OUTPUT" 2>&1

if grep -q "switch i" "$TMP_OUTPUT"; then
  echo "Did not expect jump-table switch for sparse cases" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "switch.cmp" "$TMP_OUTPUT"; then
  echo "Expected chained compares for sparse switch" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "codegen_switch_sparse test passed."
