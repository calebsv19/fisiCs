#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/include_pragma_once.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if grep -Fq "duplicated_declaration" "$TMP_OUTPUT"; then
  echo "#pragma once was ignored; duplicate branch compiled" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "INCLUDED_ONCE" "$TMP_OUTPUT" && ! grep -Fq "1" "$TMP_OUTPUT"; then
  echo "Expected INCLUDED_ONCE payload to survive preprocessing" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor #pragma once test passed."
