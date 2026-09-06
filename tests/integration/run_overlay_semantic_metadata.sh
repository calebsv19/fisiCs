#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"
if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

tmpdir="$(mktemp -d)"
cleanup() { rm -rf "$tmpdir"; }
trap cleanup EXIT

cat >"$tmpdir/valid.c" <<'SRC'
[[fisics::id("thermal.read_temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
double read_temperature(void);

[[fisics::id("thermal.limit")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.read")]]
double temperature_limit = 42.0;

[[fisics::id("thermal.read_temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
double read_temperature(void) { return temperature_limit; }
SRC

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --dump-sema \
  -c "$tmpdir/valid.c" -o "$tmpdir/valid.o" >"$tmpdir/valid.out" 2>&1
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --dump-sema \
  -c "$tmpdir/valid.c" -o "$tmpdir/valid-repeat.o" >"$tmpdir/valid-repeat.out" 2>&1
cmp "$tmpdir/valid.out" "$tmpdir/valid-repeat.out"

grep -Fq "SemanticMetadata: 2 record(s)" "$tmpdir/valid.out"
grep -Fq "  - function: read_temperature [id=thermal.read_temperature, domain=thermal, role=sensor.read, resolved]" "$tmpdir/valid.out"
grep -Fq "  - variable: temperature_limit [id=thermal.limit, domain=thermal, role=data.read, resolved]" "$tmpdir/valid.out"
if [ "$(grep -Fxc "  - function: read_temperature [id=thermal.read_temperature, domain=thermal, role=sensor.read, resolved]" "$tmpdir/valid.out")" -ne 1 ]; then
  echo "prototype and definition did not merge into one semantic metadata record" >&2
  exit 1
fi
function_line="$(grep -Fnx "  - function: read_temperature [id=thermal.read_temperature, domain=thermal, role=sensor.read, resolved]" "$tmpdir/valid.out" | cut -d: -f1)"
variable_line="$(grep -Fnx "  - variable: temperature_limit [id=thermal.limit, domain=thermal, role=data.read, resolved]" "$tmpdir/valid.out" | cut -d: -f1)"
if [ "$function_line" -ge "$variable_line" ]; then
  echo "semantic metadata dump is not ordered by first source location" >&2
  exit 1
fi

cat >"$tmpdir/plain.c" <<'SRC'
int plain_global = 4;
int plain(void) { return plain_global; }
SRC
FISICS_MAX_PROCS=0 "$BIN" -c "$tmpdir/plain.c" -o "$tmpdir/plain-default.o" >/dev/null 2>&1
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata -c "$tmpdir/plain.c" \
  -o "$tmpdir/plain-overlay.o" >/dev/null 2>&1
cmp "$tmpdir/plain-default.o" "$tmpdir/plain-overlay.o"
FISICS_MAX_PROCS=0 "$BIN" --dump-sema -c "$tmpdir/plain.c" \
  -o "$tmpdir/plain-dump.o" >"$tmpdir/plain-dump.out" 2>&1
if grep -Fq "SemanticMetadata:" "$tmpdir/plain-dump.out"; then
  echo "ordinary C semantic dump unexpectedly gained an empty metadata section" >&2
  exit 1
fi

cat >"$tmpdir/relationships.c" <<'SRC'
[[fisics::id("thermal.read_cabin_temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
[[fisics::produces("signal.cabin.temperature")]]
double read_cabin_temperature(void);

[[fisics::id("signal.cabin.temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("data.read")]]
[[fisics::consumes("environment.cabin.air")]]
double cabin_temperature = 21.0;

[[fisics::domain("control")]]
[[fisics::role("control.decide")]]
[[fisics::consumes("signal.cabin.temperature")]]
[[fisics::produces("device.roof_fan.target")]]
void control_without_id(void) {}

[[fisics::id("thermal.read_cabin_temperature")]]
[[fisics::domain("thermal")]]
[[fisics::role("sensor.read")]]
[[fisics::produces("signal.cabin.temperature")]]
[[fisics::produces("signal.cabin.humidity")]]
double read_cabin_temperature(void) { return cabin_temperature; }
SRC

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --dump-sema \
  -c "$tmpdir/relationships.c" -o "$tmpdir/relationships.o" \
  >"$tmpdir/relationships.out" 2>&1
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --dump-sema \
  -c "$tmpdir/relationships.c" -o "$tmpdir/relationships-repeat.o" \
  >"$tmpdir/relationships-repeat.out" 2>&1
cmp "$tmpdir/relationships.out" "$tmpdir/relationships-repeat.out"
grep -Fq "SemanticMetadata: 3 record(s)" "$tmpdir/relationships.out"
grep -Fq "SemanticMetadataGraph: 5 edge(s)" "$tmpdir/relationships.out"
grep -Fq "  - PRODUCES: thermal.read_cabin_temperature -> signal.cabin.temperature [resolved]" "$tmpdir/relationships.out"
grep -Fq "  - PRODUCES: thermal.read_cabin_temperature -> signal.cabin.humidity [external]" "$tmpdir/relationships.out"
grep -Fq "  - CONSUMES: signal.cabin.temperature -> environment.cabin.air [external]" "$tmpdir/relationships.out"
grep -Eq '  - CONSUMES: symbol:function:.*:control_without_id -> signal\.cabin\.temperature \[resolved\]' "$tmpdir/relationships.out"
grep -Eq '  - PRODUCES: symbol:function:.*:control_without_id -> device\.roof_fan\.target \[external\]' "$tmpdir/relationships.out"
if [ "$(grep -Fxc "  - PRODUCES: thermal.read_cabin_temperature -> signal.cabin.temperature [resolved]" "$tmpdir/relationships.out")" -ne 1 ]; then
  echo "identical relationship across compatible redeclarations did not merge" >&2
  exit 1
fi
humidity_line="$(grep -Fnx "  - PRODUCES: thermal.read_cabin_temperature -> signal.cabin.humidity [external]" "$tmpdir/relationships.out" | cut -d: -f1)"
temperature_line="$(grep -Fnx "  - PRODUCES: thermal.read_cabin_temperature -> signal.cabin.temperature [resolved]" "$tmpdir/relationships.out" | cut -d: -f1)"
if [ "$humidity_line" -ge "$temperature_line" ]; then
  echo "relationship targets are not deterministically ordered" >&2
  exit 1
fi
if grep -E '^  - (PRODUCES|CONSUMES): .*0x[0-9a-fA-F]+' "$tmpdir/relationships.out" >/dev/null; then
  echo "relationship graph leaked pointer-derived identity" >&2
  exit 1
fi

cat >"$tmpdir/disabled.c" <<'SRC'
[[fisics::id("thermal.disabled")]]
[[fisics::produces("signal.disabled")]]
int disabled(void) { return 0; }
SRC

cat >"$tmpdir/invalid.c" <<'SRC'
[[fisics::id("Thermal.Bad")]]
[[fisics::domain("unknown_domain")]]
[[fisics::role("unknown.role")]]
int invalid(void) { return 0; }

[[fisics::id()]] int missing_argument(void) { return 0; }
[[fisics::id("syntax.extra", "extra")]] int extra_argument(void) { return 0; }
[[fisics::id(not_a_string)]] int non_string(void) { return 0; }
[[fisics::id("")]] int empty_name(void) { return 0; }
[[fisics::id("bad..segment")]] int empty_segment(void) { return 0; }
[[fisics::id("bad name")]] int whitespace_name(void) { return 0; }
[[fisics::id("bad\\escape")]] int escaped_name(void) { return 0; }
[[fisics::id("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")]] int oversized_name(void) { return 0; }
[[fisics::produces("Signal.Bad")]] int invalid_relationship(void) { return 0; }
SRC

cat >"$tmpdir/local.c" <<'SRC'
struct TargetRecord {
    [[fisics::domain("thermal")]] int field;
};

[[fisics::domain("thermal")]] typedef int DeferredType;

int local_target([[fisics::role("data.read")]] int parameter) {
    [[fisics::domain("thermal")]] int local = 0;
    [[fisics::produces("signal.local")]] int local_relationship = 0;
    [[fisics::role("control.decide")]] if (parameter) local++;
    return local + local_relationship;
}
SRC

cat >"$tmpdir/multi.c" <<'SRC'
[[fisics::produces("signal.multi")]] int first, second;
SRC

cat >"$tmpdir/duplicate.c" <<'SRC'
[[fisics::domain("thermal")]]
[[fisics::domain("thermal")]]
int duplicate(void) { return 0; }
SRC

cat >"$tmpdir/redecl.c" <<'SRC'
[[fisics::domain("thermal")]] int redeclared(void);
[[fisics::domain("hardware")]] int redeclared(void) { return 0; }
SRC

cat >"$tmpdir/id_collision.c" <<'SRC'
[[fisics::id("hardware.shared")]] int first_symbol(void) { return 1; }
[[fisics::id("hardware.shared")]] int second_symbol(void) { return 2; }
SRC

cat >"$tmpdir/duplicate_relationship.c" <<'SRC'
[[fisics::produces("signal.cabin.temperature")]]
[[fisics::produces("signal.cabin.temperature")]]
int duplicate_relationship(void) { return 0; }
SRC

run_failure() {
  local name="$1"
  local expected_code="$2"
  local overlay_args=(--overlay=semantic-metadata)
  if [ "$name" = "disabled" ]; then overlay_args=(); fi
  local status=0
  FISICS_MAX_PROCS=0 "$BIN" "${overlay_args[@]}" --emit-diags-json \
    "$tmpdir/$name.json" -c "$tmpdir/$name.c" -o "$tmpdir/$name.o" \
    >"$tmpdir/$name.out" 2>&1 || status=$?
  if [ "$status" -ne 1 ]; then
    echo "$name: expected compiler exit 1, got $status" >&2
    cat "$tmpdir/$name.out" >&2
    exit 1
  fi
  python3 - "$tmpdir/$name.json" "$expected_code" <<'PY'
import json
import sys
path, expected = sys.argv[1], int(sys.argv[2])
with open(path, "r", encoding="utf-8") as fh:
    diagnostics = json.load(fh).get("diagnostics", [])
codes = {item.get("code_id") for item in diagnostics}
if expected not in codes:
    raise SystemExit(f"{path}: expected diagnostic {expected}, got {sorted(codes)}")
for item in diagnostics:
    if item.get("code_id") == expected:
        if item.get("category_name") != "extension" or item.get("stage") != "extension":
            raise SystemExit(f"{path}: incorrect taxonomy for {expected}: {item}")
        break
PY
}

run_failure disabled 4201
run_failure invalid 4202
run_failure local 4203
run_failure multi 4203
run_failure duplicate 4204
run_failure redecl 4205
run_failure id_collision 4206
run_failure duplicate_relationship 4209

cat >"$tmpdir/composed.c" <<'SRC'
[[fisics::id("physics.distance")]]
[[fisics::domain("physics")]]
[[fisics::role("data.read")]]
double distance [[fisics::dim(length)]] = 1.0;
SRC
FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata --dump-sema \
  -c "$tmpdir/composed.c" -o "$tmpdir/composed.o" >"$tmpdir/composed.out" 2>&1
grep -Fq "Extension Units Annotations: 1" "$tmpdir/composed.out"
grep -Fq "variable: distance [id=physics.distance, domain=physics, role=data.read, resolved]" "$tmpdir/composed.out"

echo "semantic-metadata overlay contract: PASS"
