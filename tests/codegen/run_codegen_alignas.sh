#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

TMP_OUTPUT=$(mktemp)
TMP_DIR=$(mktemp -d)
trap 'rm -f "$TMP_OUTPUT"; rm -rf "$TMP_DIR"' EXIT

"$BIN" tests/codegen/codegen_alignas.c > "$TMP_OUTPUT" 2>&1

if ! grep -q "@g_aligned =.*align 128" "$TMP_OUTPUT"; then
  echo "Expected global align 128 for g_aligned" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "%local = alloca .*align 32" "$TMP_OUTPUT"; then
  echo "Expected local alloca with align 32" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "%a = alloca .*align 32" "$TMP_OUTPUT"; then
  echo "Expected struct alloca with align 32" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "%AlignedUnion = type { [0 x <32 x i8>], i32, [28 x i8] }" "$TMP_OUTPUT"; then
  echo "Expected an LLVM-level 32-byte alignment anchor for AlignedUnion" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "@g_union =.*align 32" "$TMP_OUTPUT"; then
  echo "Expected global align 32 for g_union" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -q "%u = alloca %AlignedUnion, align 32" "$TMP_OUTPUT"; then
  echo "Expected local AlignedUnion alloca with align 32" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

CLANG_BIN="${CLANG:-clang}"
ABI_FIXTURES="tests/codegen/fixtures"
"$BIN" -I"$ABI_FIXTURES" -c "$ABI_FIXTURES/aligned_union_abi_lib.c" -o "$TMP_DIR/fisics_lib.o" >/dev/null 2>&1
"$CLANG_BIN" -std=c11 -I"$ABI_FIXTURES" -c "$ABI_FIXTURES/aligned_union_abi_main.c" -o "$TMP_DIR/clang_main.o"
"$CLANG_BIN" "$TMP_DIR/clang_main.o" "$TMP_DIR/fisics_lib.o" -o "$TMP_DIR/clang_calls_fisics"
if ! "$TMP_DIR/clang_calls_fisics"; then
  echo "Clang caller to fisiCs over-aligned union ABI check failed" >&2
  exit 1
fi

"$CLANG_BIN" -std=c11 -I"$ABI_FIXTURES" -c "$ABI_FIXTURES/aligned_union_abi_lib.c" -o "$TMP_DIR/clang_lib.o"
"$BIN" -I"$ABI_FIXTURES" -c "$ABI_FIXTURES/aligned_union_abi_main.c" -o "$TMP_DIR/fisics_main.o" >/dev/null 2>&1
"$CLANG_BIN" "$TMP_DIR/fisics_main.o" "$TMP_DIR/clang_lib.o" -o "$TMP_DIR/fisics_calls_clang"
if ! "$TMP_DIR/fisics_calls_clang"; then
  echo "fisiCs caller to Clang over-aligned union ABI check failed" >&2
  exit 1
fi

echo "codegen_alignas test passed."
