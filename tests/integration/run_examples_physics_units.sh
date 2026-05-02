#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
VALID_SRC="$ROOT_DIR/examples/physics_units/ballistics_valid.c"
INVALID_ASSIGN_SRC="$ROOT_DIR/examples/physics_units/ballistics_invalid_assignment.c"
INVALID_COMPARE_SRC="$ROOT_DIR/examples/physics_units/ballistics_invalid_compare.c"
INVALID_POSITION_UPDATE_SRC="$ROOT_DIR/examples/physics_units/ballistics_invalid_position_update.c"
INVALID_CHAIN_SRC="$ROOT_DIR/examples/physics_units/ballistics_invalid_formula_chain.c"
TMP_DIR=$(mktemp -d)
TMP_OUTPUT=$(mktemp)
trap 'rm -rf "$TMP_DIR" "$TMP_OUTPUT"' EXIT

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units "$VALID_SRC" -o "$TMP_DIR/ballistics_valid" >"$TMP_OUTPUT" 2>&1; then
  echo "valid physics units example failed to build normally" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$VALID_SRC" -o "$TMP_DIR/ballistics_valid.o" >"$TMP_OUTPUT" 2>&1; then
  echo "valid physics units example failed to build with overlay dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "line 4: m [resolved, unit=meter, unit-resolved]" "$TMP_OUTPUT"; then
  echo "missing position annotation in valid example dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "line 10: m^2*kg/s^2 [resolved, unit=joule, unit-resolved]" "$TMP_OUTPUT"; then
  echo "missing kinetic energy annotation in valid example dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "binary-expression(*) [dim=m/s, resolved]" "$TMP_OUTPUT"; then
  echo "missing velocity update multiply inference in valid example dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "binary-expression(*) [dim=m^2*kg/s^2, resolved]" "$TMP_OUTPUT"; then
  echo "missing kinetic energy multiply inference in valid example dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$INVALID_ASSIGN_SRC" -o "$TMP_DIR/ballistics_invalid_assignment.o" >"$TMP_OUTPUT" 2>&1; then
  echo "expected invalid assignment example to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units assignment mismatch" "$TMP_OUTPUT"; then
  echo "missing units assignment mismatch diagnostic in invalid assignment example" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "SemanticModel: 1 error(s)" "$TMP_OUTPUT"; then
  echo "invalid assignment example did not record semantic-model failure count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Skipping LLVM code generation due to semantic errors." "$TMP_OUTPUT"; then
  echo "invalid assignment example did not stop before LLVM code generation" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$INVALID_COMPARE_SRC" -o "$TMP_DIR/ballistics_invalid_compare.o" >"$TMP_OUTPUT" 2>&1; then
  echo "expected invalid comparison example to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units comparison mismatch" "$TMP_OUTPUT"; then
  echo "missing units comparison mismatch diagnostic in invalid comparison example" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "SemanticModel: 1 error(s)" "$TMP_OUTPUT"; then
  echo "invalid comparison example did not record semantic-model failure count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Skipping LLVM code generation due to semantic errors." "$TMP_OUTPUT"; then
  echo "invalid comparison example did not stop before LLVM code generation" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$INVALID_POSITION_UPDATE_SRC" -o "$TMP_DIR/ballistics_invalid_position_update.o" >"$TMP_OUTPUT" 2>&1; then
  echo "expected invalid position-update example to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units addition mismatch" "$TMP_OUTPUT"; then
  echo "missing units addition mismatch diagnostic in invalid position-update example" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(position) [dim=m, resolved, unit=meter]" "$TMP_OUTPUT"; then
  echo "invalid position-update example did not record resolved position dimension" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(acceleration) [dim=m/s^2, resolved, unit=meter_per_second_squared]" "$TMP_OUTPUT"; then
  echo "invalid position-update example did not record resolved acceleration dimension" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "SemanticModel: 1 error(s)" "$TMP_OUTPUT"; then
  echo "invalid position-update example did not record semantic-model failure count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Skipping LLVM code generation due to semantic errors." "$TMP_OUTPUT"; then
  echo "invalid position-update example did not stop before LLVM code generation" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$INVALID_CHAIN_SRC" -o "$TMP_DIR/ballistics_invalid_formula_chain.o" >"$TMP_OUTPUT" 2>&1; then
  echo "expected invalid formula-chain example to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units addition mismatch" "$TMP_OUTPUT"; then
  echo "missing units addition mismatch diagnostic in invalid formula-chain example" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "binary-expression(*) [dim=m, resolved]" "$TMP_OUTPUT"; then
  echo "invalid formula-chain example did not preserve legal intermediate multiply inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "SemanticModel: 1 error(s)" "$TMP_OUTPUT"; then
  echo "invalid formula-chain example did not record semantic-model failure count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Skipping LLVM code generation due to semantic errors." "$TMP_OUTPUT"; then
  echo "invalid formula-chain example did not stop before LLVM code generation" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "physics_units_examples integration test passed"
