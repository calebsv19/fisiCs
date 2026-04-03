#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/ifdef_keyword.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if ! grep -Fq "keyword_ok" "$TMP_OUTPUT"; then
  echo "keyword #ifndef branch missing from output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "keep_has_builtin" "$TMP_OUTPUT"; then
  echo "__has_builtin false branch missing from output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "drop_has_builtin" "$TMP_OUTPUT"; then
  echo "__has_builtin true branch leaked into output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "keep_has_extension" "$TMP_OUTPUT"; then
  echo "__has_extension fallback branch missing from output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "drop_has_extension" "$TMP_OUTPUT"; then
  echo "__has_extension enabled branch leaked into output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor #ifndef keyword + __has_builtin/__has_extension test passed."
