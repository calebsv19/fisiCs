#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./fisics}"

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

tmpdir="$(mktemp -d)"
cleanup() {
  rm -rf "$tmpdir"
}
trap cleanup EXIT

assign_src="$tmpdir/units_assign_mismatch.c"
assign_json="$tmpdir/units_assign_mismatch.json"
convert_src="$tmpdir/units_convert_incompatible.c"
convert_json="$tmpdir/units_convert_incompatible.json"
implicit_src="$tmpdir/units_implicit_convert.c"
implicit_json="$tmpdir/units_implicit_convert.json"
add_src="$tmpdir/units_add_mismatch.c"
add_json="$tmpdir/units_add_mismatch.json"
sub_src="$tmpdir/units_sub_mismatch.c"
sub_json="$tmpdir/units_sub_mismatch.json"
compare_src="$tmpdir/units_compare_mismatch.c"
compare_json="$tmpdir/units_compare_mismatch.json"

cat >"$assign_src" <<'SRC'
double distance [[fisics::dim(length)]] = 3.0;
double elapsed [[fisics::dim(time)]] = 2.0;

int main(void) {
    distance = elapsed;
    return 0;
}
SRC

cat >"$convert_src" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.0;

int main(void) {
    return (int)fisics_convert_unit(distance_m, "second");
}
SRC

cat >"$implicit_src" <<'SRC'
double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]] = 3.0;
double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]] = 0.0;

int main(void) {
    distance_ft = distance_m;
    return 0;
}
SRC

cat >"$add_src" <<'SRC'
double distance [[fisics::dim(length)]] = 3.0;
double elapsed [[fisics::dim(time)]] = 2.0;

int main(void) {
    return (int)(distance + elapsed);
}
SRC

cat >"$sub_src" <<'SRC'
double distance [[fisics::dim(length)]] = 3.0;
double elapsed [[fisics::dim(time)]] = 2.0;

int main(void) {
    return (int)(distance - elapsed);
}
SRC

cat >"$compare_src" <<'SRC'
double distance [[fisics::dim(length)]] = 3.0;
double elapsed [[fisics::dim(time)]] = 2.0;

int main(void) {
    return distance < elapsed;
}
SRC

for fixture in \
  "$assign_json:$assign_src" \
  "$convert_json:$convert_src" \
  "$implicit_json:$implicit_src" \
  "$add_json:$add_src" \
  "$sub_json:$sub_src" \
  "$compare_json:$compare_src"; do
  json_path="${fixture%%:*}"
  src_path="${fixture#*:}"
  compiler_status=0
  FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units --emit-diags-json "$json_path" "$src_path" >/dev/null 2>&1 || compiler_status=$?
  if [ "$compiler_status" -ne 1 ]; then
    echo "expected compiler exit 1 for units diagnostic fixture $src_path; got $compiler_status" >&2
    exit 1
  fi
done

python3 - "$assign_json" "$convert_json" "$implicit_json" "$add_json" "$sub_json" "$compare_json" <<'PY'
import json
import sys

assign_path, convert_path, implicit_path, add_path, sub_path, compare_path = sys.argv[1:7]

def load(path):
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)

def find_diag(path, code_name):
    data = load(path)
    for diag in data.get("diagnostics", []):
        if diag.get("code_name") == code_name:
            return diag
    raise SystemExit(f"missing {code_name} in {path}: {data!r}")

def assert_dim_details(diag, context):
    details = diag.get("details")
    if not isinstance(details, dict):
        raise SystemExit(f"{context} diagnostic missing details: {diag!r}")
    if details.get("context") != context:
        raise SystemExit(f"bad {context} context: {details!r}")
    if details.get("lhs_dim_text") != "m" or details.get("rhs_dim_text") != "s":
        raise SystemExit(f"bad {context} dim text: {details!r}")
    if details.get("lhs_dim") != [1, 0, 0, 0, 0, 0, 0, 0]:
        raise SystemExit(f"bad {context} lhs dim vector: {details!r}")
    if details.get("rhs_dim") != [0, 0, 1, 0, 0, 0, 0, 0]:
        raise SystemExit(f"bad {context} rhs dim vector: {details!r}")

assign = find_diag(assign_path, "extension.units.assign_dim_mismatch")
assert_dim_details(assign, "assignment")

add = find_diag(add_path, "extension.units.add_dim_mismatch")
assert_dim_details(add, "addition")

sub = find_diag(sub_path, "extension.units.sub_dim_mismatch")
assert_dim_details(sub, "subtraction")

compare = find_diag(compare_path, "extension.units.compare_dim_mismatch")
assert_dim_details(compare, "comparison")

convert = find_diag(convert_path, "extension.units.conversion_incompatible")
details = convert.get("details")
if not isinstance(details, dict):
    raise SystemExit(f"conversion diagnostic missing details: {convert!r}")
if details.get("context") != "explicit conversion":
    raise SystemExit(f"bad conversion context: {details!r}")
if details.get("lhs_dim_text") != "m" or details.get("rhs_dim_text") != "s":
    raise SystemExit(f"bad conversion dim text: {details!r}")
source = details.get("source_unit")
target = details.get("target_unit")
if source.get("name") != "meter" or source.get("symbol") != "m" or source.get("family") != "length":
    raise SystemExit(f"bad source unit details: {details!r}")
if target.get("name") != "second" or target.get("symbol") != "s" or target.get("family") != "time":
    raise SystemExit(f"bad target unit details: {details!r}")

implicit = find_diag(implicit_path, "extension.units.implicit_concrete_conversion")
details = implicit.get("details")
if not isinstance(details, dict):
    raise SystemExit(f"implicit conversion diagnostic missing details: {implicit!r}")
if details.get("context") != "assignment":
    raise SystemExit(f"bad implicit context: {details!r}")
source = details.get("source_unit")
target = details.get("target_unit")
if source.get("name") != "meter" or target.get("name") != "foot":
    raise SystemExit(f"bad implicit unit names: {details!r}")
if source.get("dim") != target.get("dim") or source.get("dim_text") != "m":
    raise SystemExit(f"same-family unit dims should match: {details!r}")
PY
