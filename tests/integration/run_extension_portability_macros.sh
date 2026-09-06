#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-$ROOT/fisics}"
CLANG_BIN="${CLANG:-clang}"
FIXTURES="$ROOT/tests/fixtures/extension_portability"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

if [ ! -x "$BIN" ]; then
  echo "compiler binary not found at $BIN" >&2
  exit 1
fi

emit_pair() {
  local stem="$1"
  local source="$2"
  FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
    -I"$ROOT/include" \
    --emit-semantic-metadata-json "$OUT/$stem.semantic.json" \
    --emit-units-metadata-json "$OUT/$stem.units.json" \
    -c "$source" -o "$OUT/$stem.o"
}

emit_pair native "$FIXTURES/native_equivalent.c"
emit_pair macro "$FIXTURES/macro_equivalent.c"

"$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic \
  -I"$ROOT/include" -c "$FIXTURES/macro_equivalent.c" \
  -o "$OUT/macro.clang.o"
"$CLANG_BIN" -std=c99 -E -P -I"$ROOT/include" \
  "$FIXTURES/macro_equivalent.c" >"$OUT/macro.clang.i"
if grep -Fq 'fisics::' "$OUT/macro.clang.i"; then
  echo "portable macros did not erase from Clang preprocessing output" >&2
  exit 1
fi

python3 - "$OUT" <<'PY'
import json
import pathlib
import sys

root = pathlib.Path(sys.argv[1])

def load(name):
    return json.loads((root / name).read_text())

def semantic_projection(data):
    if not data["complete"]:
        raise SystemExit("semantic parity input is incomplete")
    records = data["records"]
    names = {index: record["symbol_name"] for index, record in enumerate(records)}
    record_projection = sorted(
        (
            record["symbol_name"],
            record["symbol_kind"],
            record["authored_id"],
            record["domain"],
            record["role"],
        )
        for record in records
    )
    edge_projection = sorted(
        (
            names[edge["source_record_index"]],
            edge["kind"],
            edge["target_id"],
            edge["resolution"],
            names.get(edge.get("target_record_index")),
        )
        for edge in data["edges"]
    )
    return record_projection, edge_projection

def units_projection(data):
    if not data["complete"] or not data["overlay_enabled"]:
        raise SystemExit("units parity input is incomplete or disabled")
    return sorted(
        (
            item["symbol_name"],
            item["dimension"]["text"],
            tuple(item["dimension"]["vector"]),
            item["unit"]["source_text"],
            item["unit"]["name"],
            item["unit"]["family"],
            item["unit"]["conversion_model"],
            item["unit"]["scale_binary64"],
            item["unit"]["offset_binary64"],
        )
        for item in data["attachments"]
    )

native_semantic = semantic_projection(load("native.semantic.json"))
macro_semantic = semantic_projection(load("macro.semantic.json"))
native_units = units_projection(load("native.units.json"))
macro_units = units_projection(load("macro.units.json"))

if native_semantic != macro_semantic:
    raise SystemExit("native and macro semantic projections differ")
if native_units != macro_units:
    raise SystemExit("native and macro units projections differ")
if len(macro_semantic[0]) != 3 or len(macro_semantic[1]) != 2:
    raise SystemExit("portable semantic parity fixture coverage changed")
if len(macro_units) != 2:
    raise SystemExit("portable units parity fixture coverage changed")
PY

compare_program() {
  local name="$1"
  local overlay="$2"
  local expected="$3"
  shift 3
  local sources=("$@")

  FISICS_MAX_PROCS=0 "$BIN" "$overlay" -I"$ROOT/include" \
    "${sources[@]}" -o "$OUT/$name.fisics"
  "$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic \
    -I"$ROOT/include" "${sources[@]}" -o "$OUT/$name.clang"
  "$OUT/$name.fisics" >"$OUT/$name.fisics.stdout"
  "$OUT/$name.clang" >"$OUT/$name.clang.stdout"
  cmp "$OUT/$name.fisics.stdout" "$OUT/$name.clang.stdout"
  cmp "$expected" "$OUT/$name.fisics.stdout"
}

printf 'position=1.702 velocity=17.019 energy=181.029\n' >"$OUT/ballistics.expected"
compare_program ballistics --overlay=physics-units "$OUT/ballistics.expected" \
  "$ROOT/examples/physics_units/ballistics_valid.c"

printf 'temperature=294.0 fan_target=0\n' >"$OUT/metadata-basic.expected"
compare_program metadata-basic --overlay=semantic-metadata \
  "$OUT/metadata-basic.expected" \
  "$ROOT/examples/semantic_metadata/basic/main.c"

for source in "$ROOT"/examples/physics_units/ballistics_invalid_*.c; do
  stem="$(basename "${source%.c}")"
  "$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic \
    -I"$ROOT/include" -c "$source" -o "$OUT/$stem.clang.o"
done

for source in \
  "$ROOT/examples/semantic_metadata/hardware_project/controller.c" \
  "$ROOT/examples/semantic_metadata/hardware_project/signals.c"; do
  stem="$(basename "${source%.c}")"
  "$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic \
    -I"$ROOT/include" -c "$source" -o "$OUT/hardware-$stem.clang.o"
done

for project in kinematics_stepper collision_1d; do
  project_root="$ROOT/examples/projects/$project"
  compare_program "$project" --overlay=physics-units \
    "$project_root/tests/expected_stdout.txt" \
    -I"$project_root/src" \
    "$project_root/src/main.c" "$project_root/src/$project.c"

  for source in "$project_root"/tests/invalid/*.c; do
    stem="$(basename "${source%.c}")"
    "$CLANG_BIN" -std=c99 -Wall -Wextra -Werror -pedantic \
      -I"$ROOT/include" -I"$project_root/src" \
      -c "$source" -o "$OUT/$project-$stem.clang.o"
  done
done

echo "extension portability macro integration test passed"
