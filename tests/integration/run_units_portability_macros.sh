#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-$ROOT/fisics}"
CLANG_BIN="${CLANG:-clang}"
FIXTURES="$ROOT/tests/fixtures/units_portability"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units \
  -I"$ROOT/include" --dump-sema \
  "$FIXTURES/native_equivalent.c" -o "$OUT/native.fisics" \
  >"$OUT/native.sema" 2>&1
FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units \
  -I"$ROOT/include" --dump-sema \
  "$FIXTURES/macro_equivalent.c" -o "$OUT/macro.fisics" \
  >"$OUT/macro.sema" 2>&1

for sema in "$OUT/native.sema" "$OUT/macro.sema"; do
  grep -Eq 'm \[resolved, unit=meter, unit-resolved\]' "$sema"
  grep -Eq 'm/s \[resolved, unit=meter_per_second, unit-resolved\]' "$sema"
  grep -Fq 'SemanticModel: 0 error(s)' "$sema"
done

"$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" "$FIXTURES/macro_equivalent.c" -o "$OUT/macro.clang"
"$CLANG_BIN" -std=c99 -E -P -I"$ROOT/include" \
  "$FIXTURES/macro_equivalent.c" >"$OUT/macro.clang.i"
if grep -Fq 'fisics::' "$OUT/macro.clang.i"; then
  echo "physics-units portability macros did not erase under Clang" >&2
  exit 1
fi

"$OUT/native.fisics" >"$OUT/native.stdout"
"$OUT/macro.fisics" >"$OUT/macro.fisics.stdout"
"$OUT/macro.clang" >"$OUT/macro.clang.stdout"
cmp "$OUT/native.stdout" "$OUT/macro.fisics.stdout"
cmp "$OUT/native.stdout" "$OUT/macro.clang.stdout"
grep -Fxq 'distance=2.000 speed=3.000' "$OUT/native.stdout"

echo "physics-units portability macro integration test passed"
