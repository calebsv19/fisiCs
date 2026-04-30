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

cat > "$TMP_DIR/overlay_deferred_unit_word.c" <<'SRC'
double elapsed [[fisics::dim(second)]] = 1.0;
int main(void) {
    return (int)elapsed;
}
SRC

cat > "$TMP_DIR/overlay_duplicate.c" <<'SRC'
double distance [[fisics::dim(m)]] [[fisics::dim(s)]] = 10.0;
int main(void) {
    return (int)distance;
}
SRC

cat > "$TMP_DIR/overlay_unit_ok.c" <<'SRC'
double distance [[fisics::dim(length)]] [[fisics::unit(meter)]] = 10.0;
double duration [[fisics::dim(duration)]] [[fisics::unit(second)]] = 2.0;
double speed [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]] = 0.0;

int main(void) {
    double derived [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]] = distance / duration;
    return (int)derived;
}
SRC

cat > "$TMP_DIR/overlay_unit_convert_ok.c" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.048;
double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]] = 0.0;

int main(void) {
    double converted [[fisics::dim(length)]] [[fisics::unit(foot)]] =
        fisics_convert_unit(distance_m, "foot");
    distance_ft = fisics_convert_unit(distance_m, "foot");
    return (int)(converted + distance_ft);
}
SRC

cat > "$TMP_DIR/overlay_builtin_unit_convert_ok.c" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.048;

int main(void) {
    double converted [[fisics::dim(length)]] [[fisics::unit(foot)]] =
        __builtin_fisics_convert_unit(distance_m, "foot");
    return (int)converted;
}
SRC

cat > "$TMP_DIR/overlay_unit_assign_requires_convert.c" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.048;
double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]] = 0.0;

int main(void) {
    distance_ft = distance_m;
    return (int)distance_ft;
}
SRC

cat > "$TMP_DIR/overlay_unit_add_requires_convert.c" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.048;
double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]] = 0.0;

int main(void) {
    return (int)(distance_m + distance_ft);
}
SRC

cat > "$TMP_DIR/overlay_unit_convert_invalid_target.c" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.048;

int main(void) {
    return (int)fisics_convert_unit(distance_m, "bogus_unit_name");
}
SRC

cat > "$TMP_DIR/overlay_unit_convert_incompatible.c" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.048;

int main(void) {
    return (int)fisics_convert_unit(distance_m, "second");
}
SRC

cat > "$TMP_DIR/overlay_unit_convert_missing_source.c" <<'SRC'
double distance [[fisics::dim(length)]] = 3.048;

int main(void) {
    return (int)fisics_convert_unit(distance, "foot");
}
SRC

cat > "$TMP_DIR/overlay_unit_convert_requires_floating.c" <<'SRC'
int elapsed [[fisics::dim(time)]] [[fisics::unit(second)]] = 60;

int main(void) {
    return fisics_convert_unit(elapsed, "minute");
}
SRC

cat > "$TMP_DIR/overlay_unit_without_dim.c" <<'SRC'
double elapsed [[fisics::unit(second)]] = 1.0;
int main(void) {
    return (int)elapsed;
}
SRC

cat > "$TMP_DIR/overlay_unit_mismatch.c" <<'SRC'
double distance [[fisics::dim(length)]] [[fisics::unit(second)]] = 1.0;
int main(void) {
    return (int)distance;
}
SRC

cat > "$TMP_DIR/overlay_duplicate_unit.c" <<'SRC'
double elapsed [[fisics::dim(time)]] [[fisics::unit(second)]] [[fisics::unit(minute)]] = 1.0;
int main(void) {
    return (int)elapsed;
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

cat > "$TMP_DIR/overlay_alias_ok.c" <<'SRC'
double travel [[fisics::dim(distance)]] = 10.0;
double duration [[fisics::dim(duration)]] = 2.0;
double speed [[fisics::dim(speed)]] = 1.0;
double kinetic [[fisics::dim(kinetic_energy)]] = 0.0;

int main(void) {
    double derived_speed [[fisics::dim(speed)]] = travel / duration;
    double accel [[fisics::dim(acceleration)]] = derived_speed / duration;
    return (int)(derived_speed + speed + accel * duration);
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

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_alias_ok.c" -o "$TMP_DIR/overlay_alias_ok.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay alias vocabulary compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: travel [units=m, resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing canonicalized travel alias symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: duration [units=s, resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing canonicalized duration alias symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: speed [units=m/s, resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing canonicalized speed alias symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: kinetic [units=m^2*kg/s^2, resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing canonicalized kinetic alias symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "binary-expression(/) [dim=m/s, resolved]" "$TMP_OUTPUT"; then
  echo "Missing alias-driven divide inference" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_ok.c" -o "$TMP_DIR/overlay_unit_ok.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay unit annotation compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: distance [units=m, resolved, unit=meter, unit-resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing distance unit attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: duration [units=s, resolved, unit=second, unit-resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing duration unit attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: speed [units=m/s, resolved, unit=foot_per_second, unit-resolved, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing speed unit attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "unit=meter_per_second, unit-resolved" "$TMP_OUTPUT"; then
  echo "Missing derived local unit attachment dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_convert_ok.c" -o "$TMP_DIR/overlay_unit_convert_ok.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay explicit unit conversion compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "function-call(fisics_convert_unit) [dim=m, resolved, unit=foot]" "$TMP_OUTPUT"; then
  echo "Missing explicit conversion function-call units dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "assignment(=) [dim=m, resolved, unit=foot]" "$TMP_OUTPUT"; then
  echo "Missing explicit conversion assignment units dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_builtin_unit_convert_ok.c" -o "$TMP_DIR/overlay_builtin_unit_convert_ok.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay builtin explicit unit conversion compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "function-call(__builtin_fisics_convert_unit) [dim=m, resolved, unit=foot]" "$TMP_OUTPUT"; then
  echo "Missing builtin explicit conversion function-call units dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_assign_requires_convert.c" -o "$TMP_DIR/overlay_unit_assign_requires_convert.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected same-dimension assignment unit mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "implicit concrete unit conversion in assignment requires explicit conversion from 'meter' to 'foot'" "$TMP_OUTPUT"; then
  echo "Missing same-dimension assignment unit-conversion diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_add_requires_convert.c" -o "$TMP_DIR/overlay_unit_add_requires_convert.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected same-dimension addition unit mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "implicit concrete unit conversion in addition requires explicit conversion from 'foot' to 'meter'" "$TMP_OUTPUT"; then
  echo "Missing same-dimension addition unit-conversion diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_convert_invalid_target.c" -o "$TMP_DIR/overlay_unit_convert_invalid_target.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected invalid explicit conversion compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "invalid explicit units conversion target 'bogus_unit_name'" "$TMP_OUTPUT"; then
  echo "Missing invalid explicit conversion target diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_convert_incompatible.c" -o "$TMP_DIR/overlay_unit_convert_incompatible.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected incompatible explicit conversion compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "explicit units conversion from 'meter' to 'second' is not allowed" "$TMP_OUTPUT"; then
  echo "Missing incompatible explicit conversion diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_convert_missing_source.c" -o "$TMP_DIR/overlay_unit_convert_missing_source.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected explicit conversion without source unit compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "requires a resolved source concrete unit" "$TMP_OUTPUT"; then
  echo "Missing explicit conversion source-unit diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_convert_requires_floating.c" -o "$TMP_DIR/overlay_unit_convert_requires_floating.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected explicit conversion non-floating source compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "currently requires a floating scalar source expression" "$TMP_OUTPUT"; then
  echo "Missing explicit conversion floating-source diagnostic" >&2
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

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_deferred_unit_word.c" -o "$TMP_DIR/overlay_deferred_unit_word.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected deferred unit word compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "concrete unit word 'second' belongs in [[fisics::unit(...)]]" "$TMP_OUTPUT"; then
  echo "Missing deferred unit-word diagnostic detail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: elapsed [units=second, pending, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing deferred-unit pending symbol attachment dump" >&2
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

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_without_dim.c" -o "$TMP_DIR/overlay_unit_without_dim.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected unit-without-dim compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "requires a matching fisics::dim annotation" "$TMP_OUTPUT"; then
  echo "Missing unit-without-dim diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: elapsed [units=<no-dim>, pending, unit=second, unit-pending, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing unit-without-dim pending symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_unit_mismatch.c" -o "$TMP_DIR/overlay_unit_mismatch.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected unit-dimension mismatch compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "fisics::unit expression 'second' has dimension 's' but declaration dim is 'm'" "$TMP_OUTPUT"; then
  echo "Missing unit-dimension mismatch diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: distance [units=m, resolved, unit=second, unit-pending, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing unit-dimension mismatch symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --dump-sema -c "$TMP_DIR/overlay_duplicate_unit.c" -o "$TMP_DIR/overlay_duplicate_unit.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected duplicate unit annotation compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "duplicate fisics::unit annotation" "$TMP_OUTPUT"; then
  echo "Missing duplicate unit diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "variable: elapsed [units=s, resolved, unit=second, unit-pending, unit-duplicates=2, decl-index=0]" "$TMP_OUTPUT"; then
  echo "Missing duplicate unit symbol dump" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "overlay_units_scaffold integration test passed"
