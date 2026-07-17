#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

OUT=$(mktemp)
BIN_OUT=$(mktemp)
trap 'rm -f "$OUT" "$BIN_OUT"' EXIT

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

# Bitfields sharing one storage unit are packed into the first carrier:
# a=5 occupies bits 0..2 and b=7 occupies bits 3..7, yielding 61.
if ! grep -q "@gb = .*%Bit { i32 61, i32 0 }" "$OUT"; then
  echo "bitfield initializer missing" >&2
  cat "$OUT" >&2
  exit 1
fi

# Keep the IR-shape assertion tied to observable behavior.
"$BIN" tests/codegen/codegen_const_globals.c -o "$BIN_OUT" >/dev/null 2>&1
set +e
"$BIN_OUT"
STATUS=$?
set -e
if [ "$STATUS" -ne 13 ]; then
  echo "constant-global runtime result incorrect: expected 13, got $STATUS" >&2
  exit 1
fi

echo "codegen_const_globals test passed."
