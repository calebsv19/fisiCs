#!/usr/bin/env bash
set -euo pipefail

RUNTIME_LIB="${1:-build/unsanitized/libfisics_memcheck_runtime.a}"
RUNTIME_INCLUDE_DIR="${2:-runtime/memory_check}"

if [ ! -f "$RUNTIME_LIB" ]; then
  echo "memory-check runtime library not found at $RUNTIME_LIB" >&2
  exit 1
fi

if [ ! -f "$RUNTIME_INCLUDE_DIR/fisics_memcheck_runtime.h" ]; then
  echo "memory-check runtime header not found in $RUNTIME_INCLUDE_DIR" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$TMP_OUTPUT"' EXIT

cat > "$TMP_DIR/memcheck_runtime_skeleton.c" <<'SRC'
#include "fisics_memcheck_runtime.h"

int main(void) {
    __fisics_memcheck_reset();

    void *first = __fisics_memcheck_malloc(8);
    if (!first) return 2;

    first = __fisics_memcheck_realloc(first, 16);
    if (!first) return 3;

    void *leaked = __fisics_memcheck_calloc(3, 4);
    if (!leaked) return 4;

    __fisics_memcheck_free(first);
    __fisics_memcheck_free(0);
    __fisics_memcheck_free(first);

    int stack_value = 0;
    __fisics_memcheck_free(&stack_value);

    __fisics_memcheck_report();
    (void)leaked;
    return 0;
}
SRC

cc -Wall -Wextra -Wpedantic -I"$RUNTIME_INCLUDE_DIR" \
  "$TMP_DIR/memcheck_runtime_skeleton.c" "$RUNTIME_LIB" \
  -o "$TMP_DIR/memcheck_runtime_skeleton"

if ! "$TMP_DIR/memcheck_runtime_skeleton" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check runtime skeleton executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] double free" "$TMP_OUTPUT"; then
  echo "missing double-free diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] unknown pointer free" "$TMP_OUTPUT"; then
  echo "missing unknown-pointer diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] summary: active=1 leaked_bytes=12 allocs=3 frees=2 double_free=1 unknown_free=1 tracker_failures=0" "$TMP_OUTPUT"; then
  echo "missing expected memory-check summary" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "memory_check_runtime_skeleton integration test passed"
