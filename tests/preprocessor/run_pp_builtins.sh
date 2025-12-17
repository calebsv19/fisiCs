#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
SRC="tests/preprocessor/builtin_macros.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" "$SRC" > "$TMP_OUTPUT"

if ! grep -Fq "STRING_LITERAL \"virt.c\"" "$TMP_OUTPUT"; then
  echo "__FILE__ remap did not surface expected virtual filename" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "STRING_LITERAL \"tests/preprocessor/builtin_macros.c\"" "$TMP_OUTPUT"; then
  echo "__BASE_FILE__ missing from output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "NUMBER_LITERAL 300" "$TMP_OUTPUT"; then
  echo "__LINE__ remap did not produce expected constant (300)" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "IDENTIFIER vaopt_with" "$TMP_OUTPUT" || ! grep -Fq "NUMBER_LITERAL 10" "$TMP_OUTPUT" || ! grep -Fq "NUMBER_LITERAL 2" "$TMP_OUTPUT"; then
  echo "__VA_OPT__ with args did not emit payload" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "IDENTIFIER vaopt_without" "$TMP_OUTPUT"; then
  echo "__VA_OPT__ without args missing" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "IDENTIFIER counter_third" "$TMP_OUTPUT" || ! grep -Fq "NUMBER_LITERAL 2" "$TMP_OUTPUT"; then
  echo "__COUNTER__ sequence unexpected at third use" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "IDENTIFIER counter_fourth" "$TMP_OUTPUT" || ! grep -Fq "NUMBER_LITERAL 3" "$TMP_OUTPUT"; then
  echo "__COUNTER__ sequence unexpected at fourth use" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor built-in macro test passed."
