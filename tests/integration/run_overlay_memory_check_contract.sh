#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$TMP_OUTPUT"' EXIT

cat > "$TMP_DIR/memory_check_flag_only.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(4);
    free(p);
    return 0;
}
SRC

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check -c "$TMP_DIR/memory_check_flag_only.c" -o "$TMP_DIR/memory_check_flag_only.o" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check overlay CLI compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$TMP_DIR/memory_check_flag_only.o" ]; then
  echo "expected object file missing for memory-check overlay compile" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 FISICS_OVERLAY=memory_check "$BIN" -c "$TMP_DIR/memory_check_flag_only.c" -o "$TMP_DIR/memory_check_env.o" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check overlay env compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=not-a-real-overlay -c "$TMP_DIR/memory_check_flag_only.c" -o "$TMP_DIR/bad.o" >"$TMP_OUTPUT" 2>&1; then
  echo "unknown overlay unexpectedly succeeded" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "unknown overlay mode" "$TMP_OUTPUT"; then
  echo "missing unknown-overlay diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "overlay_memory_check_contract integration test passed"
