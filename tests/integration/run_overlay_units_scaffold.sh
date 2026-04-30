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

cat > "$TMP_DIR/overlay_ok.c" <<'SRC'
double gravity [[fisics::dim(acceleration)]] = 9.8;

struct MotionSample {
    double accel [[fisics::dim(m/s^2)]];
};

static double sample_distance(double distance [[fisics::dim(m)]]) {
    double offset [[fisics::dim(m)]] = 1.0;
    return distance + offset;
}

int main(void) {
    struct MotionSample sample = {0};
    return (int)(sample.accel + sample_distance(2.0));
}
SRC

cat > "$TMP_DIR/overlay_disabled.c" <<'SRC'
double distance [[fisics::dim(m)]] = 10.0;
int main(void) {
    return (int)distance;
}
SRC

cat > "$TMP_DIR/overlay_invalid.c" <<'SRC'
double accel [[fisics::dim(m^x)]] = 9.8;
int main(void) {
    return (int)accel;
}
SRC

cat > "$TMP_DIR/overlay_duplicate.c" <<'SRC'
double distance [[fisics::dim(m)]] [[fisics::dim(s)]] = 10.0;
int main(void) {
    return (int)distance;
}
SRC

cat > "$TMP_DIR/overlay_expr_infer.c" <<'SRC'
double distance [[fisics::dim(m)]] = 10.0;
double time_step [[fisics::dim(s)]] = 2.0;
double velocity [[fisics::dim(m/s)]] = 1.0;
double fallback_velocity [[fisics::dim(m/s)]] = 4.0;

int main(void) {
    double selected [[fisics::dim(m/s)]] =
        (fallback_velocity > velocity) ? (distance / time_step) : velocity;
    return (selected < fallback_velocity) ? 1 : 0;
}
SRC

cat > "$TMP_DIR/overlay_assign_mismatch.c" <<'SRC'
double distance [[fisics::dim(m)]] = 10.0;
double time_step [[fisics::dim(s)]] = 2.0;

int main(void) {
    distance = time_step;
    return (int)distance;
}
SRC

cat > "$TMP_DIR/overlay_add_mismatch.c" <<'SRC'
double distance [[fisics::dim(m)]] = 10.0;
double time_step [[fisics::dim(s)]] = 2.0;

int main(void) {
    return (int)(distance + time_step);
}
SRC

cat > "$TMP_DIR/overlay_compare_mismatch.c" <<'SRC'
double distance [[fisics::dim(m)]] = 10.0;
double time_step [[fisics::dim(s)]] = 2.0;

int main(void) {
    return (distance < time_step) ? 1 : 0;
}
SRC

cat > "$TMP_DIR/overlay_ternary_mismatch.c" <<'SRC'
double distance [[fisics::dim(m)]] = 10.0;
double time_step [[fisics::dim(s)]] = 2.0;

int main(void) {
    return (int)((distance > distance) ? distance : time_step);
}
SRC

cat > "$TMP_DIR/overlay_exponent_overflow.c" <<'SRC'
double huge_length [[fisics::dim(m^127)]] = 1.0;
double unit_length [[fisics::dim(m)]] = 2.0;

int main(void) {
    return (int)(huge_length * unit_length);
}
SRC

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_ok.c" -o "$TMP_DIR/overlay_ok.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay-enabled units scaffold compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$TMP_DIR/overlay_ok.o" ]; then
  echo "Expected object file missing for overlay-enabled compile" >&2
  exit 1
fi

if ! grep -Fq "Extension Units Annotations: 4" "$TMP_OUTPUT"; then
  echo "Missing semantic dump units annotation count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "m/s^2 [resolved]" "$TMP_OUTPUT"; then
  echo "Missing semantic dump canonical units annotation" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: gravity [units=m/s^2, resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing symbol-level units attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "number-literal [dim=m/s^2, resolved]" "$TMP_OUTPUT"; then
  echo "Missing declaration-owned literal units result" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(distance) [dim=m, resolved]" "$TMP_OUTPUT"; then
  echo "Missing identifier units inference for parameter binding" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(offset) [dim=m, resolved]" "$TMP_OUTPUT"; then
  echo "Missing identifier units inference for local declaration binding" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "number-literal [dim=1, resolved]" "$TMP_OUTPUT"; then
  echo "Missing dimensionless literal units result" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_expr_infer.c" -o "$TMP_DIR/overlay_expr_infer.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay expression inference compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(distance) [dim=m, resolved]" "$TMP_OUTPUT"; then
  echo "Missing global identifier units inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(time_step) [dim=s, resolved]" "$TMP_OUTPUT"; then
  echo "Missing second global identifier units inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "identifier(velocity) [dim=m/s, resolved]" "$TMP_OUTPUT"; then
  echo "Missing velocity identifier units inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "binary-expression(/) [dim=m/s, resolved]" "$TMP_OUTPUT"; then
  echo "Missing divide expression units inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "ternary-expression [dim=m/s, resolved]" "$TMP_OUTPUT"; then
  echo "Missing ternary expression units inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "binary-expression(<) [dim=1, resolved]" "$TMP_OUTPUT"; then
  echo "Missing comparison expression units inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_assign_mismatch.c" -o "$TMP_DIR/overlay_assign_mismatch.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected assignment mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units assignment mismatch" "$TMP_OUTPUT"; then
  echo "Missing assignment mismatch diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_add_mismatch.c" -o "$TMP_DIR/overlay_add_mismatch.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected addition mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units addition mismatch" "$TMP_OUTPUT"; then
  echo "Missing addition mismatch diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_compare_mismatch.c" -o "$TMP_DIR/overlay_compare_mismatch.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected comparison mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units comparison mismatch" "$TMP_OUTPUT"; then
  echo "Missing comparison mismatch diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_ternary_mismatch.c" -o "$TMP_DIR/overlay_ternary_mismatch.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected ternary mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units ternary mismatch" "$TMP_OUTPUT"; then
  echo "Missing ternary mismatch diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_exponent_overflow.c" -o "$TMP_DIR/overlay_exponent_overflow.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected exponent overflow compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "units exponent overflow" "$TMP_OUTPUT"; then
  echo "Missing exponent overflow diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --dump-sema -c "$TMP_DIR/overlay_disabled.c" -o "$TMP_DIR/overlay_disabled.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected overlay-disabled compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "requires the physics-units overlay" "$TMP_OUTPUT"; then
  echo "Missing overlay-disabled diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: distance [units=m, pending, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing overlay-disabled pending symbol attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Extension Units Annotations: 1" "$TMP_OUTPUT"; then
  echo "Missing overlay-disabled units annotation count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "Extension Units Expr Results: 1" "$TMP_OUTPUT"; then
  echo "Missing overlay-disabled expression-result count" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_invalid.c" -o "$TMP_DIR/overlay_invalid.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected invalid units expression compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "invalid fisics::dim expression" "$TMP_OUTPUT"; then
  echo "Missing invalid units diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: accel [units=m^x, pending, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing invalid-expression pending symbol attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "m^x [pending]" "$TMP_OUTPUT"; then
  echo "Missing invalid-expression pending annotation dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_duplicate.c" -o "$TMP_DIR/overlay_duplicate.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected duplicate units annotation compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "duplicate fisics::dim annotation" "$TMP_OUTPUT"; then
  echo "Missing duplicate units diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: distance [units=m, pending, duplicates=2, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing duplicate pending symbol attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "m [pending, duplicates=2]" "$TMP_OUTPUT"; then
  echo "Missing duplicate pending annotation dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "overlay_units_scaffold integration test passed"
