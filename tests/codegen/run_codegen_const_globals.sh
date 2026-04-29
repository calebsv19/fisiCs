#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

OUT=$(mktemp)
trap 'rm -f "$OUT"' EXIT

"$BIN" tests/codegen/codegen_const_globals.c >"$OUT" 2>&1

# String literals into arrays and pointers
if ! grep -q "@msg = .*\\[3 x i8\\] c\\\"hi\\\\00\\\"" "$OUT"; then
  echo "Missing or incorrect msg initializer" >&2
  cat "$OUT" >&2
  exit 1
fi
if ! grep -q "@padded = .*\\[5 x i8\\] c\\\"abc\\\\00\\\\00\\\"" "$OUT"; then
  echo "Missing padded char array initializer" >&2
  cat "$OUT" >&2
  exit 1
fi
if ! grep -Eq "@ptr = .*(global|constant) ptr @\\.str\\.0" "$OUT"; then
  echo "Pointer-to-string initializer not emitted" >&2
  cat "$OUT" >&2
  exit 1
fi

# Array zero-fill
if ! grep -q "@ints = .*\[4 x i32\] \[i32 1, i32 2, i32 0, i32 0\]" "$OUT"; then
  echo "ints array initializer incorrect" >&2
  cat "$OUT" >&2
  exit 1
fi

# Struct initializers
if ! grep -q "@gp = .*%P { i32 1, i32 2 }" "$OUT"; then
  echo "struct gp initializer missing" >&2
  cat "$OUT" >&2
  exit 1
fi
if ! grep -q "@gp2 = .*%P { i32 3, i32 4 }" "$OUT"; then
  echo "static struct gp2 initializer missing" >&2
  cat "$OUT" >&2
  exit 1
fi

# Bitfield carrier should still be a constant struct initializer
if ! grep -q "@gb = .*%Bit { i32 5, i32 7 }" "$OUT"; then
  echo "bitfield initializer missing" >&2
  cat "$OUT" >&2
  exit 1
fi

echo "codegen_const_globals test passed."
