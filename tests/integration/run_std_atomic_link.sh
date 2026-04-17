#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
OUT="${2:-/tmp/fisics-std-atomic-link}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$OUT" "$TMP_OUTPUT"' EXIT

cat > "$TMP_DIR/main.c" <<'SRC'
#include <stdatomic.h>

int main(void) {
    _Atomic int counter;
    atomic_init(&counter, 1);

    int loaded = atomic_load_explicit(&counter, memory_order_acquire);
    atomic_store_explicit(&counter, loaded + 1, memory_order_release);

    int old = atomic_exchange_explicit(&counter, 7, memory_order_acq_rel);
    int final = atomic_load_explicit(&counter, memory_order_relaxed);

    return (old == 2 && final == 7) ? 0 : 1;
}
SRC

if ! "$BIN" -std=c11 "$TMP_DIR/main.c" -o "$OUT" >"$TMP_OUTPUT" 2>&1; then
  echo "-std=c11 atomic compile+link failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -x "$OUT" ]; then
  echo "Expected executable missing: $OUT" >&2
  exit 1
fi

if grep -Fq "___c11_atomic" "$TMP_OUTPUT"; then
  echo "Unexpected unresolved c11 atomic symbols in output" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

"$OUT"

echo "std_atomic_link integration test passed: $OUT"
