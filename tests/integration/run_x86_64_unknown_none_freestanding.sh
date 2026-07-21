#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPILER="${1:-$ROOT/fisics}"
SOURCE="$ROOT/tests/integration/cases/x86_64_unknown_none_freestanding.c"
LLVM_BINDIR="$(llvm-config --bindir)"
READOBJ="$LLVM_BINDIR/llvm-readobj"
OBJDUMP="$LLVM_BINDIR/llvm-objdump"
WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

COMPILER="$(cd "$(dirname "$COMPILER")" && pwd)/$(basename "$COMPILER")"
OBJECT_A="$WORKDIR/probe-a.o"
OBJECT_B="$WORKDIR/probe-b.o"

"$COMPILER" --target x86_64-unknown-none -c "$SOURCE" -o "$OBJECT_A"
"$COMPILER" --target x86_64-unknown-none -c "$SOURCE" -o "$OBJECT_B"
cmp "$OBJECT_A" "$OBJECT_B"

python3 "$ROOT/tests/integration/verify_x86_64_unknown_none_object.py" \
  "$READOBJ" "$OBJDUMP" "$OBJECT_A"

SOURCE_SHA256="$(shasum -a 256 "$SOURCE" | awk '{print $1}')"
OBJECT_A_SHA256="$(shasum -a 256 "$OBJECT_A" | awk '{print $1}')"
OBJECT_B_SHA256="$(shasum -a 256 "$OBJECT_B" | awk '{print $1}')"
COMPILER_SHA256="$(shasum -a 256 "$COMPILER" | awk '{print $1}')"

test "$OBJECT_A_SHA256" = "$OBJECT_B_SHA256"
echo "PASS identity source_sha256=$SOURCE_SHA256"
echo "PASS identity object_sha256=$OBJECT_A_SHA256 repeat_object_sha256=$OBJECT_B_SHA256"
echo "PASS identity compiler_sha256=$COMPILER_SHA256"
echo "x86_64-unknown-none freestanding object contract: PASS"
