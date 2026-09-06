#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-./fisics}"
cd "$ROOT"

if [ ! -x "$BIN" ]; then
  echo "ERROR: compiler binary is not executable: $BIN" >&2
  exit 2
fi

tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/fisics-semantic-metadata-json.XXXXXX")"
trap 'rm -rf "$tmpdir"' EXIT
fixture="tests/fixtures/semantic_metadata_sm5/relationships.c"

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --emit-semantic-metadata-json "$tmpdir/one.json" -c "$fixture" -o "$tmpdir/with.o" >"$tmpdir/valid.out" 2>"$tmpdir/valid.err"
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --emit-semantic-metadata-json "$tmpdir/two.json" -c "$fixture" -o "$tmpdir/repeat.o" >"$tmpdir/repeat.out" 2>"$tmpdir/repeat.err"
cmp "$tmpdir/one.json" "$tmpdir/two.json"
cmp "$tmpdir/with.o" "$tmpdir/repeat.o"

cc -std=c99 -Wall -Wextra -Wpedantic $(llvm-config --cflags) \
  -Isrc -Isrc/Lexer -Isrc/Parser -Isrc/Syntax -Isrc/Syntax/Expr \
  -Isrc/Syntax/Decls -Isrc/CodeGen -Isrc/CodeGen/Expr \
  -Ithird_party/codework_shared/core/core_base/include \
  -Ithird_party/codework_shared/core/core_io/include \
  -Ithird_party/codework_shared/core/core_data/include \
  -Ithird_party/codework_shared/core/core_pack/include \
  tests/integration/semantic_metadata_frontend_json_parity.c \
  -L. -lfisics_frontend -o "$tmpdir/frontend-parity" \
  $(llvm-config --ldflags --libs core native nativecodegen x86codegen passes)
FISICS_MAX_PROCS=0 "$tmpdir/frontend-parity" "$fixture" "$tmpdir/frontend.json" \
  >"$tmpdir/frontend.out" 2>"$tmpdir/frontend.err"
cmp "$tmpdir/one.json" "$tmpdir/frontend.json"

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata -c "$fixture" -o "$tmpdir/without.o" >"$tmpdir/plain.out" 2>"$tmpdir/plain.err"
cmp "$tmpdir/with.o" "$tmpdir/without.o"

cat >"$tmpdir/link.c" <<'C'
[[fisics::id("program.main")]]
[[fisics::domain("control")]]
[[fisics::role("control.decide")]]
int main(void) { return 0; }
C
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/link.json" "$tmpdir/link.c" \
  -o "$tmpdir/link" >"$tmpdir/link.out" 2>"$tmpdir/link.err"
"$tmpdir/link"
test -s "$tmpdir/link.json"

python3 - "$tmpdir/one.json" tests/fixtures/semantic_metadata_sm5/relationships.expected.json.in VERSION <<'PY'
import json
import pathlib
import sys

artifact_path = pathlib.Path(sys.argv[1])
template_path = pathlib.Path(sys.argv[2])
version_path = pathlib.Path(sys.argv[3])
raw = artifact_path.read_bytes()
if not raw.endswith(b"\n") or raw.endswith(b"\n\n"):
    raise SystemExit("semantic metadata JSON must have exactly one final newline")
document = json.loads(raw)
if list(document) != ["schema", "schema_version", "producer", "analysis_contract", "source", "complete", "records", "edges"]:
    raise SystemExit("unexpected top-level key order")
if len(document["records"]) != 3 or len(document["edges"]) != 4:
    raise SystemExit("unexpected projection cardinality")

records = {record["symbol_name"]: record for record in document["records"]}
read = records["read_environment"]
temperature = records["cabin_temperature"]
control = records["control_without_id"]
replacements = {
    "@PRODUCER_VERSION@": version_path.read_text().strip(),
    "@SOURCE_PATH@": document["source"]["path"],
    "@SOURCE_HASH@": document["source"]["hash_fnv1a64"],
    "@SOURCE_LENGTH@": str(document["source"]["length_bytes"]),
    "@READ_ENVIRONMENT_STABLE_ID@": read["symbol_stable_id"],
    "@CABIN_TEMPERATURE_STABLE_ID@": temperature["symbol_stable_id"],
    "@CONTROL_STABLE_ID@": control["symbol_stable_id"],
    "@READ_ENVIRONMENT_COLUMN@": str(read["source"]["start_column"]),
    "@READ_ENVIRONMENT_END_COLUMN@": str(read["source"]["end_column"]),
    "@CABIN_TEMPERATURE_COLUMN@": str(temperature["source"]["start_column"]),
    "@CABIN_TEMPERATURE_END_COLUMN@": str(temperature["source"]["end_column"]),
    "@CONTROL_COLUMN@": str(control["source"]["start_column"]),
    "@CONTROL_END_COLUMN@": str(control["source"]["end_column"]),
}
expected = template_path.read_text()
for token, value in replacements.items():
    expected = expected.replace(token, value)
if "@" in expected:
    raise SystemExit("unsubstituted fixture token")
if raw != expected.encode():
    raise SystemExit("semantic metadata JSON differs from exact frozen template")
PY

cat >"$tmpdir/empty.c" <<'C'
int ordinary_value = 1;
C
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --emit-semantic-metadata-json "$tmpdir/empty.json" -c "$tmpdir/empty.c" -o "$tmpdir/empty.o" >"$tmpdir/empty.out" 2>"$tmpdir/empty.err"
python3 - "$tmpdir/empty.json" <<'PY'
import json, sys
document = json.load(open(sys.argv[1]))
assert document["complete"] is True
assert document["records"] == []
assert document["edges"] == []
PY

cat >"$tmpdir/invalid.c" <<'C'
[[fisics::id("duplicate.identity")]] int first_value;
[[fisics::id("duplicate.identity")]] int second_value;
C
printf 'preserve-existing\n' >"$tmpdir/preserved.json"
if FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --emit-semantic-metadata-json "$tmpdir/preserved.json" -c "$tmpdir/invalid.c" -o "$tmpdir/invalid.o" >"$tmpdir/invalid.out" 2>"$tmpdir/invalid.err"; then
  echo "ERROR: invalid metadata unexpectedly compiled" >&2
  exit 1
fi
grep -qx 'preserve-existing' "$tmpdir/preserved.json"

printf 'preserve-allocation-failure\n' >"$tmpdir/allocation-failure.json"
if FISICS_TEST_FAIL_SEMANTIC_METADATA_PROJECTION_ALLOC=1 FISICS_MAX_PROCS=0 \
  "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/allocation-failure.json" -c "$fixture" \
  -o "$tmpdir/allocation-failure.o" >/dev/null 2>"$tmpdir/allocation-failure.err"; then
  echo "ERROR: injected projection allocation failure unexpectedly succeeded" >&2
  exit 1
fi
grep -qx 'preserve-allocation-failure' "$tmpdir/allocation-failure.json"

mkdir "$tmpdir/destination-directory"
if FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/destination-directory" -c "$fixture" \
  -o "$tmpdir/rename-failure.o" >/dev/null 2>"$tmpdir/rename-failure.err"; then
  echo "ERROR: atomic replacement over a directory unexpectedly succeeded" >&2
  exit 1
fi
test -d "$tmpdir/destination-directory"

printf 'preserve-missing-overlay\n' >"$tmpdir/missing-overlay.json"
if FISICS_MAX_PROCS=0 "$BIN" --emit-semantic-metadata-json "$tmpdir/missing-overlay.json" -c "$fixture" -o "$tmpdir/missing-overlay.o" >/dev/null 2>"$tmpdir/missing-overlay.err"; then
  echo "ERROR: metadata JSON without overlay unexpectedly succeeded" >&2
  exit 1
fi
grep -q 'requires --overlay=semantic-metadata' "$tmpdir/missing-overlay.err"
grep -qx 'preserve-missing-overlay' "$tmpdir/missing-overlay.json"

printf 'preserve-multi-input\n' >"$tmpdir/multi-input.json"
if FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --emit-semantic-metadata-json "$tmpdir/multi-input.json" -c "$fixture" "$tmpdir/empty.c" >/dev/null 2>"$tmpdir/multi-input.err"; then
  echo "ERROR: multi-input metadata JSON unexpectedly succeeded" >&2
  exit 1
fi
grep -q 'requires exactly one C input' "$tmpdir/multi-input.err"
grep -qx 'preserve-multi-input' "$tmpdir/multi-input.json"

if FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata --emit-semantic-metadata-json - -c "$fixture" -o "$tmpdir/dash.o" >/dev/null 2>"$tmpdir/dash-path.err"; then
  echo "ERROR: stdout metadata path unexpectedly succeeded" >&2
  exit 1
fi
grep -q "'-' is not supported" "$tmpdir/dash-path.err"

if FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/manifest.json" \
  --build-manifest tests/integration/fixtures/build_manifest_valid.json "$fixture" \
  >/dev/null 2>"$tmpdir/manifest.err"; then
  echo "ERROR: manifest metadata JSON unexpectedly succeeded" >&2
  exit 1
fi
grep -q 'does not support build manifests' "$tmpdir/manifest.err"

if find "$tmpdir" -name '*.tmp.*' -print -quit | grep -q .; then
  echo "ERROR: semantic metadata temporary file was not cleaned up" >&2
  exit 1
fi

echo "Semantic metadata JSON contract passed"
