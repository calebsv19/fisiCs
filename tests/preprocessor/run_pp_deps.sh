#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
SRC="tests/preprocessor/include_basic.c"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

OUT_JSON=$(mktemp)
trap 'rm -f "$OUT_JSON"' EXIT

EMIT_DEPS_JSON="$OUT_JSON" "$BIN" "$SRC" >/dev/null

if [ ! -s "$OUT_JSON" ]; then
  echo "Dependency JSON not written" >&2
  exit 1
fi

if ! grep -Fq "include_basic.c" "$OUT_JSON"; then
  echo "Missing root source in dependency JSON" >&2
  cat "$OUT_JSON" >&2
  exit 1
fi

if ! grep -Fq "include_basic_local.h" "$OUT_JSON"; then
  echo "Missing local include edge in dependency JSON" >&2
  cat "$OUT_JSON" >&2
  exit 1
fi

if ! grep -Fq "include/pp_sys.h" "$OUT_JSON"; then
  echo "Missing system include edge in dependency JSON" >&2
  cat "$OUT_JSON" >&2
  exit 1
fi

echo "preprocessor dependency JSON emission test passed."
