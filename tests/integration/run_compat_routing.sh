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

cat > "$TMP_DIR/block_ptr.c" <<'SRC'
#ifndef __FISICS_EXTENSIONS__
#error "__FISICS_EXTENSIONS__ should be defined when compatibility features are enabled"
#endif

typedef int (^AdderBlock)(int);
AdderBlock g_adder;
SRC

cat > "$TMP_DIR/atomic_relaxed.c" <<'SRC'
#ifndef __FISICS_EXTENSIONS__
#error "__FISICS_EXTENSIONS__ should be defined when compatibility features are enabled"
#endif

_Atomic int g_counter;
int main(void) {
    g_counter = 1;
    return g_counter;
}
SRC

if ! FISICS_MAX_PROCS=0 "$BIN" -std=c99 --compat=block-pointers -c "$TMP_DIR/block_ptr.c" -o "$TMP_DIR/block_ptr.o" >"$TMP_OUTPUT" 2>&1; then
  echo "--compat=block-pointers compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$TMP_DIR/block_ptr.o" ]; then
  echo "Expected object file missing for --compat=block-pointers" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" -std=c99 --extensions=gnu -c "$TMP_DIR/block_ptr.c" -o "$TMP_DIR/block_ptr_alias.o" >"$TMP_OUTPUT" 2>&1; then
  echo "legacy --extensions=gnu alias compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$TMP_DIR/block_ptr_alias.o" ]; then
  echo "Expected object file missing for --extensions=gnu alias" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" -std=c99 --compat=relaxed-atomic -c "$TMP_DIR/atomic_relaxed.c" -o "$TMP_DIR/atomic_relaxed.o" >"$TMP_OUTPUT" 2>&1; then
  echo "--compat=relaxed-atomic compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$TMP_DIR/atomic_relaxed.o" ]; then
  echo "Expected object file missing for --compat=relaxed-atomic" >&2
  exit 1
fi

if grep -Fq "_Atomic is not supported yet" "$TMP_OUTPUT"; then
  echo "Unexpected _Atomic unsupported diagnostic with relaxed-atomic compatibility" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "compat_routing integration test passed"
