#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
OUT_DIR="compilation/out"
OUT_BIN="$OUT_DIR/single_bin"

mkdir -p "$OUT_DIR"

echo "[single] compiling..."
MallocNanoZone=0 "$BIN" compilation/single.c -o "$OUT_BIN"

echo "[single] running..."
if MallocNanoZone=0 "$OUT_BIN"; then
  echo "[single] success"
else
  echo "[single] binary returned non-zero" >&2
  exit 1
fi
