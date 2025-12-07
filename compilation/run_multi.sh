#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./compiler}"
OUT_DIR="compilation/out"
OUT_BIN="$OUT_DIR/multi_bin"

mkdir -p "$OUT_DIR"

echo "[multi] compiling..."
MallocNanoZone=0 "$BIN" compilation/multi_main.c compilation/multi_helper.c -o "$OUT_BIN"

echo "[multi] running..."
if MallocNanoZone=0 "$OUT_BIN"; then
  echo "[multi] success"
else
  echo "[multi] binary returned non-zero" >&2
  exit 1
fi
