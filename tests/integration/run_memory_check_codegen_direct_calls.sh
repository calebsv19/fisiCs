#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
RUNTIME_LIB="${2:-build/unsanitized/libfisics_memcheck_runtime.a}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

if [ ! -f "$RUNTIME_LIB" ]; then
  echo "memory-check runtime library not found at $RUNTIME_LIB" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$TMP_OUTPUT"' EXIT

cat > "$TMP_DIR/memory_check_direct_calls.c" <<'SRC'
#include <stdlib.h>

int main(void) {
    void *p = malloc(8);
    if (!p) return 2;
    p = realloc(p, 16);
    if (!p) return 3;
    void *leaked = calloc(3, 4);
    if (!leaked) return 4;
    free(p);
    free(0);
    free(p);
    (void)leaked;
    return 0;
}
SRC

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=memory-check -c \
  "$TMP_DIR/memory_check_direct_calls.c" \
  -o "$TMP_DIR/memory_check_direct_calls.o" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check codegen compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

for sym in __fisics_memcheck_malloc_site __fisics_memcheck_realloc_site __fisics_memcheck_calloc_site __fisics_memcheck_free_site; do
  if ! nm -u "$TMP_DIR/memory_check_direct_calls.o" | grep -Fq "$sym"; then
    echo "expected rewritten symbol missing: $sym" >&2
    nm -u "$TMP_DIR/memory_check_direct_calls.o" >&2 || true
    exit 1
  fi
done

cc "$TMP_DIR/memory_check_direct_calls.o" "$RUNTIME_LIB" -o "$TMP_DIR/memory_check_direct_calls"

if ! "$TMP_DIR/memory_check_direct_calls" >"$TMP_OUTPUT" 2>&1; then
  echo "memory-check manually-linked executable failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] double free" "$TMP_OUTPUT"; then
  echo "missing rewritten double-free diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check]   free_site=memory_check_direct_calls.c:12" "$TMP_OUTPUT"; then
  echo "missing rewritten free-site diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "[fisics:memory-check] leak: size=12 allocated_at=memory_check_direct_calls.c:8" "$TMP_OUTPUT"; then
  echo "missing rewritten allocation-site leak diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" -c \
  "$TMP_DIR/memory_check_direct_calls.c" \
  -o "$TMP_DIR/memory_check_default.o" >"$TMP_OUTPUT" 2>&1; then
  echo "default compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if nm -u "$TMP_DIR/memory_check_default.o" | grep -Fq "__fisics_memcheck"; then
  echo "default compile unexpectedly references memory-check wrappers" >&2
  nm -u "$TMP_DIR/memory_check_default.o" >&2 || true
  exit 1
fi

echo "memory_check_codegen_direct_calls integration test passed"
