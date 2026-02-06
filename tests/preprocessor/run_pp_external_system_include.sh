#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

if ! command -v clang >/dev/null 2>&1; then
  echo "clang not found; skipping external preprocessor system include test."
  exit 0
fi

SRC="tests/preprocessor/external_system_include.c"
TMP_OUTPUT=$(mktemp)

FISICS_PREPROCESS=external \
FISICS_EXTERNAL_CPP=clang \
FISICS_EXTERNAL_CPP_ARGS="-E -P" \
DISABLE_CODEGEN=1 \
"$BIN" "$SRC" >"$TMP_OUTPUT" 2>&1 || {
  cat "$TMP_OUTPUT" >&2
  rm -f "$TMP_OUTPUT"
  exit 1
}

if ! grep -q "Semantic analysis: no issues found." "$TMP_OUTPUT"; then
  echo "Expected semantic analysis success for external preprocessing path" >&2
  cat "$TMP_OUTPUT" >&2
  rm -f "$TMP_OUTPUT"
  exit 1
fi

rm -f "$TMP_OUTPUT"
echo "external preprocessor system include test passed."
