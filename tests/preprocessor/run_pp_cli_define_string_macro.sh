#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" '-DVK_RENDERER_SHADER_ROOT="$(VK_RENDERER_RESOLVED_DIR)"' \
  tests/preprocessor/cli_define_string_macro.c >"$TMP_OUTPUT" 2>&1

if grep -Fq "Undeclared identifier" "$TMP_OUTPUT"; then
  echo "Unexpected undeclared identifier with -D string macro" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "Error:" "$TMP_OUTPUT"; then
  echo "Unexpected compilation error with -D string macro" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor CLI -D string macro test passed."
