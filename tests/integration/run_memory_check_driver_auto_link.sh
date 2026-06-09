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

cat > "$TMP_DIR/memory_check_driver_link.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    free(p);
    return 0;
}
SRC

cat > "$TMP_DIR/memory_check_driver_default.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    free(p);
    return 0;
}
SRC

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check \
  "$TMP_DIR/memory_check_driver_link.c" \
  -o "$TMP_DIR/memory_check_driver_link" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check driver auto-link compile/link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! "$TMP_DIR/memory_check_driver_link" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check driver auto-linked executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] double free" "$TMP_OUTPUT"; then
  echo "missing auto-linked double-free diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check -c \
  "$TMP_DIR/memory_check_driver_link.c" \
  -o "$TMP_DIR/memory_check_driver_link.o" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check compile-only mode failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

for sym in __fisics_memcheck_malloc_site __fisics_memcheck_free_site; do
  if ! nm -u "$TMP_DIR/memory_check_driver_link.o" | grep -Fq "$sym"; then
    echo "compile-only object missing expected memory-check symbol: $sym" >&2
    nm -u "$TMP_DIR/memory_check_driver_link.o" >&2 || true
    exit 1
  fi
done

if ! FISICS_MAX_PROCS=0 "$BIN" \
  "$TMP_DIR/memory_check_driver_default.c" \
  -o "$TMP_DIR/memory_check_driver_default" >"$TMP_OUTPUT" 2>&1; then
  echo "default driver link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if "$TMP_DIR/memory_check_driver_default" >"$TMP_OUTPUT" 2>&1; then
  if grep -Fq "[fisics:memory-check]" "$TMP_OUTPUT"; then
    echo "default executable unexpectedly emitted memory-check diagnostics" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
else
  echo "default executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=all \
  "$TMP_DIR/memory_check_driver_default.c" \
  -o "$TMP_DIR/memory_check_driver_all" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay=all driver link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if "$TMP_DIR/memory_check_driver_all" >"$TMP_OUTPUT" 2>&1; then
  if grep -Fq "[fisics:memory-check]" "$TMP_OUTPUT"; then
    echo "overlay=all unexpectedly emitted memory-check diagnostics" >&2
    cat "$TMP_OUTPUT" >&2
    exit 1
  fi
else
  echo "overlay=all executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "memory_check_driver_auto_link integration test passed"
