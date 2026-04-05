#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

if ! command -v clang >/dev/null 2>&1; then
  echo "clang not found; skipping codegen_weak_linkage test."
  exit 0
fi

TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

"$BIN" -c tests/codegen/weak_linkage_weak.c -o "$TMP_DIR/weak.o" >"$TMP_DIR/weak.log" 2>&1
"$BIN" -c tests/codegen/weak_linkage_strong.c -o "$TMP_DIR/strong.o" >"$TMP_DIR/strong.log" 2>&1

clang "$TMP_DIR/weak.o" "$TMP_DIR/strong.o" tests/codegen/weak_linkage_main.c -o "$TMP_DIR/weak_linkage.out" >"$TMP_DIR/link.log" 2>&1

set +e
"$TMP_DIR/weak_linkage.out" >/dev/null 2>&1
RC=$?
set -e

if [ "$RC" -ne 14 ]; then
  echo "codegen weak linkage test failed (expected rc=14, got rc=$RC)" >&2
  cat "$TMP_DIR/weak.log" >&2
  cat "$TMP_DIR/strong.log" >&2
  cat "$TMP_DIR/link.log" >&2
  exit 1
fi

echo "codegen_weak_linkage test passed."
