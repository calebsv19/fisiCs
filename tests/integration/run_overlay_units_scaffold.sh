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
struct MotionSample {
    double accel [[fisics::dim(m/s^2)]];
};

static double sample_distance(double distance [[fisics::dim(m)]]) {
    double force [[fisics::dim(force)]] = 1.0;
    return distance + force;
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

if ! FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units -c "$TMP_DIR/overlay_ok.c" -o "$TMP_DIR/overlay_ok.o" >"$TMP_OUTPUT" 2>&1; then
  echo "overlay-enabled units scaffold compile failed" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if [ ! -f "$TMP_DIR/overlay_ok.o" ]; then
  echo "Expected object file missing for overlay-enabled compile" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" -c "$TMP_DIR/overlay_disabled.c" -o "$TMP_DIR/overlay_disabled.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected overlay-disabled compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "requires the physics-units overlay" "$TMP_OUTPUT"; then
  echo "Missing overlay-disabled diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units -c "$TMP_DIR/overlay_invalid.c" -o "$TMP_DIR/overlay_invalid.o" >"$TMP_OUTPUT" 2>&1; then
  echo "Expected invalid units expression compile to fail" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

if ! grep -Fq "invalid fisics::dim expression" "$TMP_OUTPUT"; then
  echo "Missing invalid units diagnostic" >&2
  cat "$TMP_OUTPUT" >&2
  exit 1
fi

echo "overlay_units_scaffold integration test passed"
