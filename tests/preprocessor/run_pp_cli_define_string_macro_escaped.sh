#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
trap 'rm -f "$TMP_OUTPUT"' EXIT

"$BIN" '-DVK_RENDERER_SHADER_ROOT=\"$(VK_RENDERER_RESOLVED_DIR)\"' \
  '-DVK_RENDERER_SHADER_ROOT=\"/Users/calebsv/Desktop/CodeWork/shared/vk_renderer\"' \
  tests/preprocessor/cli_define_string_macro_escaped.c >"$TMP_OUTPUT" 2>&1

if grep -Fq "Unexpected token at start of expression" "$TMP_OUTPUT"; then
  echo "Unexpected parse error with escaped -D string macro" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if grep -Fq "Error:" "$TMP_OUTPUT"; then
  echo "Unexpected compilation error with escaped -D string macro" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "preprocessor CLI escaped -D string macro test passed."
