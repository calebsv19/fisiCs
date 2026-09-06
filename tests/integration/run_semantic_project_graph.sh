#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-./fisics}"
cd "$ROOT"

if [ ! -x "$BIN" ]; then
  echo "ERROR: compiler binary is not executable: $BIN" >&2
  exit 2
fi

tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/fisics-semantic-project.XXXXXX")"
trap 'rm -rf "$tmpdir"' EXIT

controller_source="tests/fixtures/semantic_metadata_sm6/controller.c"
sensor_source="tests/fixtures/semantic_metadata_sm6/sensor.c"
self_source="tests/fixtures/semantic_metadata_sm6/self_cycle.c"

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/controller.json" \
  -c "$controller_source" -o "$tmpdir/controller.o" >/dev/null
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/sensor.json" \
  -c "$sensor_source" -o "$tmpdir/sensor.o" >/dev/null
cp "$tmpdir/controller.o" "$tmpdir/controller.before.o"

python3 - "$tmpdir/manifest.json" "$tmpdir/controller.json" "$tmpdir/sensor.json" <<'PY'
import json, pathlib, sys
path, controller, sensor = map(pathlib.Path, sys.argv[1:])
path.write_text(json.dumps({
    "schema": "fisiCs.semantic_project_manifest",
    "schema_version": 1,
    "project_id": "van.control",
    "translation_units": [
        {"metadata_path": controller.name},
        {"metadata_path": sensor.name},
    ],
    "allowed_external_targets": [
        "service.weather.forecast",
        "device.cabin.fan.command",
    ],
}, indent=2) + "\n")
PY

FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
  --emit-semantic-project-graph-json "$tmpdir/one.json"
FISICS_MAX_PROCS=0 "$BIN" --emit-semantic-project-graph-json "$tmpdir/two.json" \
  --semantic-project-manifest "$tmpdir/manifest.json"
cmp "$tmpdir/one.json" "$tmpdir/two.json"
cmp "$tmpdir/controller.o" "$tmpdir/controller.before.o"

python3 - "$tmpdir/one.json" tests/fixtures/semantic_metadata_sm6/project.expected.json.in \
  "$tmpdir/controller.json" "$tmpdir/sensor.json" VERSION <<'PY'
import json, pathlib, sys
graph_path = pathlib.Path(sys.argv[1])
template_path = pathlib.Path(sys.argv[2])
controller_artifact = pathlib.Path(sys.argv[3]).resolve()
sensor_artifact = pathlib.Path(sys.argv[4]).resolve()
version = pathlib.Path(sys.argv[5]).read_text().strip()
raw = graph_path.read_bytes()
if not raw.endswith(b"\n") or raw.endswith(b"\n\n"):
    raise SystemExit("project graph must have exactly one final newline")
document = json.loads(raw)
if list(document) != ["schema", "schema_version", "producer", "project", "complete",
                     "translation_units", "allowed_external_targets", "records", "edges"]:
    raise SystemExit("unexpected project graph top-level key order")
if document["schema"] != "fisiCs.semantic_project_graph" or not document["complete"]:
    raise SystemExit("unexpected project graph identity/completeness")
if len(document["translation_units"]) != 2 or len(document["records"]) != 3 or len(document["edges"]) != 3:
    raise SystemExit("unexpected project graph cardinality")
records = {record["symbol_name"]: record for record in document["records"]}
units = {pathlib.Path(unit["source"]["path"]).name: unit for unit in document["translation_units"]}
controller = units["controller.c"]
sensor = units["sensor.c"]
replacements = {
    "@PRODUCER_VERSION@": version,
    "@MANIFEST_PATH@": str(pathlib.Path(document["project"]["manifest_path"])),
    "@CONTROLLER_SOURCE_PATH@": controller["source"]["path"],
    "@CONTROLLER_HASH@": controller["source"]["hash_fnv1a64"],
    "@CONTROLLER_LENGTH@": str(controller["source"]["length_bytes"]),
    "@CONTROLLER_ARTIFACT_PATH@": str(controller_artifact),
    "@SENSOR_SOURCE_PATH@": sensor["source"]["path"],
    "@SENSOR_HASH@": sensor["source"]["hash_fnv1a64"],
    "@SENSOR_LENGTH@": str(sensor["source"]["length_bytes"]),
    "@SENSOR_ARTIFACT_PATH@": str(sensor_artifact),
    "@CONTROLLER_STABLE_ID@": records["update_cabin_control"]["symbol_stable_id"],
    "@SENSOR_STABLE_ID@": records["cabin_temperature"]["symbol_stable_id"],
    "@PUBLISH_STABLE_ID@": records["publish_temperature"]["symbol_stable_id"],
}
expected = template_path.read_text()
for token, value in replacements.items():
    expected = expected.replace(token, value)
if "@" in expected:
    raise SystemExit("unsubstituted SM-6 golden token")
if raw != expected.encode():
    raise SystemExit("semantic project graph differs from exact frozen golden")
PY

# Manifest and artifact array ordering are not semantic.
cp "$tmpdir/controller.json" "$tmpdir/controller.original.json"
cp "$tmpdir/sensor.json" "$tmpdir/sensor.original.json"
python3 - "$tmpdir/manifest.json" "$tmpdir/controller.json" "$tmpdir/sensor.json" <<'PY'
import json, pathlib, sys
manifest = pathlib.Path(sys.argv[1])
data = json.loads(manifest.read_text())
data["translation_units"].reverse()
manifest.write_text(json.dumps(data, separators=(",", ":")) + "\n")
for raw_path in sys.argv[2:]:
    path = pathlib.Path(raw_path)
    artifact = json.loads(path.read_text())
    old_count = len(artifact["records"])
    mapping = {old: old_count - 1 - old for old in range(old_count)}
    artifact["records"].reverse()
    for edge in artifact["edges"]:
        edge["source_record_index"] = mapping[edge["source_record_index"]]
        if edge["target_record_index"] is not None:
            edge["target_record_index"] = mapping[edge["target_record_index"]]
    artifact["edges"].reverse()
    path.write_text(json.dumps(artifact, separators=(",", ":")) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
  --emit-semantic-project-graph-json "$tmpdir/permuted.json"
cmp "$tmpdir/one.json" "$tmpdir/permuted.json"
cp "$tmpdir/controller.original.json" "$tmpdir/controller.json"
cp "$tmpdir/sensor.original.json" "$tmpdir/sensor.json"

expect_failure_preserves() {
  local manifest="$1"
  local needle="$2"
  local label="$3"
  printf 'preserve-%s\n' "$label" >"$tmpdir/$label.output"
  if FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$manifest" \
       --emit-semantic-project-graph-json "$tmpdir/$label.output" \
       >"$tmpdir/$label.stdout" 2>"$tmpdir/$label.stderr"; then
    echo "ERROR: $label unexpectedly succeeded" >&2
    exit 1
  fi
  grep -q "$needle" "$tmpdir/$label.stderr"
  grep -qx "preserve-$label" "$tmpdir/$label.output"
}

python3 - "$tmpdir/missing.json" "$tmpdir/controller.json" "$tmpdir/sensor.json" <<'PY'
import json, pathlib, sys
path = pathlib.Path(sys.argv[1])
path.write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.control",
 "translation_units":[{"metadata_path":sys.argv[2]},{"metadata_path":sys.argv[3]}],
 "allowed_external_targets":[]}) + "\n")
PY
expect_failure_preserves "$tmpdir/missing.json" "missing required semantic target device.cabin.fan.command" "missing"

printf 'preserve-allocation\n' >"$tmpdir/allocation.output"
if FISICS_TEST_FAIL_SEMANTIC_PROJECT_GRAPH_ALLOC=1 FISICS_MAX_PROCS=0 \
     "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
     --emit-semantic-project-graph-json "$tmpdir/allocation.output" \
     >"$tmpdir/allocation.stdout" 2>"$tmpdir/allocation.stderr"; then
  echo "ERROR: injected project graph allocation failure unexpectedly succeeded" >&2
  exit 1
fi
grep -q "injected semantic project graph allocation failure" "$tmpdir/allocation.stderr"
grep -qx 'preserve-allocation' "$tmpdir/allocation.output"

printf 'preserve-write\n' >"$tmpdir/write.output"
if FISICS_TEST_FAIL_SEMANTIC_PROJECT_GRAPH_WRITE=1 FISICS_MAX_PROCS=0 \
     "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
     --emit-semantic-project-graph-json "$tmpdir/write.output" \
     >"$tmpdir/write.stdout" 2>"$tmpdir/write.stderr"; then
  echo "ERROR: injected project graph write failure unexpectedly succeeded" >&2
  exit 1
fi
grep -q "failed to atomically publish semantic project graph" "$tmpdir/write.stderr"
grep -qx 'preserve-write' "$tmpdir/write.output"

python3 - "$tmpdir/sensor.original.json" "$tmpdir/duplicate-id.json" \
  "$tmpdir/duplicate-id-manifest.json" "$tmpdir/controller.json" <<'PY'
import json, pathlib, sys
source, artifact_path, manifest_path, controller = map(pathlib.Path, sys.argv[1:])
artifact = json.loads(source.read_text())
artifact["records"][0]["authored_id"] = "method.cabin.control"
artifact["records"][0]["source_identity"] = "method.cabin.control"
for edge in artifact["edges"]:
    if edge["target_record_index"] == 0:
        edge["target_id"] = "method.cabin.control"
artifact_path.write_text(json.dumps(artifact) + "\n")
manifest_path.write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.control",
 "translation_units":[{"metadata_path":str(controller)},{"metadata_path":str(artifact_path)}],
 "allowed_external_targets":["device.cabin.fan.command"]}) + "\n")
PY
expect_failure_preserves "$tmpdir/duplicate-id-manifest.json" "duplicate project authored_id method.cabin.control" "duplicate-id"

cp "$controller_source" "$tmpdir/stale.c"
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/stale.json" -c "$tmpdir/stale.c" \
  -o "$tmpdir/stale.o" >/dev/null
printf '\n' >>"$tmpdir/stale.c"
python3 - "$tmpdir/stale-manifest.json" "$tmpdir/stale.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.stale",
 "translation_units":[{"metadata_path":sys.argv[2]}],"allowed_external_targets":[]}) + "\n")
PY
expect_failure_preserves "$tmpdir/stale-manifest.json" "stale semantic metadata source length" "stale"

python3 - "$tmpdir/controller.original.json" "$tmpdir/bad-index.json" \
  "$tmpdir/bad-index-manifest.json" <<'PY'
import json, pathlib, sys
source, artifact_path, manifest_path = map(pathlib.Path, sys.argv[1:])
artifact = json.loads(source.read_text())
artifact["edges"][0]["resolution"] = "resolved"
artifact["edges"][0]["target_record_index"] = 999
artifact_path.write_text(json.dumps(artifact) + "\n")
manifest_path.write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.invalid",
 "translation_units":[{"metadata_path":str(artifact_path)}],
 "allowed_external_targets":["device.cabin.fan.command","signal.cabin.temperature"]}) + "\n")
PY
expect_failure_preserves "$tmpdir/bad-index-manifest.json" "out-of-range local record index" "bad-index"

python3 - "$tmpdir/controller.original.json" "$tmpdir/incomplete.json" \
  "$tmpdir/incomplete-manifest.json" <<'PY'
import json, pathlib, sys
source, artifact_path, manifest_path = map(pathlib.Path, sys.argv[1:])
artifact = json.loads(source.read_text()); artifact["complete"] = False
artifact_path.write_text(json.dumps(artifact) + "\n")
manifest_path.write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.invalid",
 "translation_units":[{"metadata_path":str(artifact_path)}],
 "allowed_external_targets":[]}) + "\n")
PY
expect_failure_preserves "$tmpdir/incomplete-manifest.json" "incomplete or exceeds" "incomplete"

python3 - "$tmpdir/controller.original.json" "$tmpdir/wrong-version.json" \
  "$tmpdir/wrong-version-manifest.json" <<'PY'
import json, pathlib, sys
source, artifact_path, manifest_path = map(pathlib.Path, sys.argv[1:])
artifact = json.loads(source.read_text()); artifact["schema_version"] = 2
artifact_path.write_text(json.dumps(artifact) + "\n")
manifest_path.write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.invalid",
 "translation_units":[{"metadata_path":str(artifact_path)}],
 "allowed_external_targets":[]}) + "\n")
PY
expect_failure_preserves "$tmpdir/wrong-version-manifest.json" "requires schema v1" "wrong-version"

cp "$controller_source" "$tmpdir/missing-source.c"
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/missing-source.json" \
  -c "$tmpdir/missing-source.c" -o "$tmpdir/missing-source.o" >/dev/null
python3 - "$tmpdir/missing-source-manifest.json" "$tmpdir/missing-source.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.missing_source",
 "translation_units":[{"metadata_path":sys.argv[2]}],
 "allowed_external_targets":["device.cabin.fan.command","signal.cabin.temperature"]}) + "\n")
PY
rm "$tmpdir/missing-source.c"
expect_failure_preserves "$tmpdir/missing-source-manifest.json" "source path is missing or not canonical" "missing-source"

python3 - "$tmpdir/duplicate-artifact-manifest.json" "$tmpdir/controller.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.duplicate",
 "translation_units":[{"metadata_path":sys.argv[2]},{"metadata_path":sys.argv[2]}],
 "allowed_external_targets":[]}) + "\n")
PY
expect_failure_preserves "$tmpdir/duplicate-artifact-manifest.json" "duplicate semantic metadata artifact path" "duplicate-artifact"

cp "$tmpdir/controller.original.json" "$tmpdir/controller-copy.json"
python3 - "$tmpdir/duplicate-source-manifest.json" "$tmpdir/controller.json" "$tmpdir/controller-copy.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.duplicate_source",
 "translation_units":[{"metadata_path":sys.argv[2]},{"metadata_path":sys.argv[3]}],
 "allowed_external_targets":["device.cabin.fan.command","signal.cabin.temperature"]}) + "\n")
PY
expect_failure_preserves "$tmpdir/duplicate-source-manifest.json" "duplicate semantic project source path" "duplicate-source"

printf '{"schema":"fisiCs.semantic_project_manifest","schema":"duplicate","schema_version":1,"project_id":"van.bad","translation_units":[],"allowed_external_targets":[]}\n' >"$tmpdir/duplicate-key.json"
expect_failure_preserves "$tmpdir/duplicate-key.json" "duplicate object key 'schema'" "duplicate-key"

python3 - "$tmpdir/unknown-field.json" "$tmpdir/controller.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.bad",
 "translation_units":[{"metadata_path":sys.argv[2]}],"allowed_external_targets":[],
 "unexpected":True}) + "\n")
PY
expect_failure_preserves "$tmpdir/unknown-field.json" "must contain exactly" "unknown-field"

python3 - "$tmpdir/oversized-manifest.json" <<'PY'
import pathlib, sys
path = pathlib.Path(sys.argv[1])
with path.open("wb") as stream:
    stream.seek(1024 * 1024)
    stream.write(b"x")
PY
expect_failure_preserves "$tmpdir/oversized-manifest.json" "exceeds 1048576 bytes" "oversized-manifest"

python3 - "$tmpdir/oversized-artifact.json" "$tmpdir/oversized-artifact-manifest.json" <<'PY'
import json, pathlib, sys
artifact, manifest = map(pathlib.Path, sys.argv[1:])
with artifact.open("wb") as stream:
    stream.seek(64 * 1024 * 1024)
    stream.write(b"x")
manifest.write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.oversized",
 "translation_units":[{"metadata_path":str(artifact)}],"allowed_external_targets":[]}) + "\n")
PY
expect_failure_preserves "$tmpdir/oversized-artifact-manifest.json" "exceeds 67108864 bytes" "oversized-artifact"

printf 'int ordinary_value = 1;\n' >"$tmpdir/empty.c"
FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/empty.json" -c "$tmpdir/empty.c" \
  -o "$tmpdir/empty.o" >/dev/null
python3 - "$tmpdir/empty-manifest.json" "$tmpdir/empty.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.empty",
 "translation_units":[{"metadata_path":sys.argv[2]}],"allowed_external_targets":[]}) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/empty-manifest.json" \
  --emit-semantic-project-graph-json "$tmpdir/empty-graph.json"
python3 - "$tmpdir/empty-graph.json" <<'PY'
import json, sys
data=json.load(open(sys.argv[1])); assert len(data["translation_units"]) == 1
assert data["records"] == [] and data["edges"] == []
PY

FISICS_MAX_PROCS=0 "$BIN" --overlay=semantic-metadata \
  --emit-semantic-metadata-json "$tmpdir/self.json" -c "$self_source" \
  -o "$tmpdir/self.o" >/dev/null
python3 - "$tmpdir/self-manifest.json" "$tmpdir/self.json" <<'PY'
import json, pathlib, sys
pathlib.Path(sys.argv[1]).write_text(json.dumps({
 "schema":"fisiCs.semantic_project_manifest","schema_version":1,"project_id":"van.self",
 "translation_units":[{"metadata_path":sys.argv[2]}],"allowed_external_targets":[]}) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/self-manifest.json" \
  --emit-semantic-project-graph-json "$tmpdir/self-graph.json"
python3 - "$tmpdir/self-graph.json" <<'PY'
import json, sys
data=json.load(open(sys.argv[1])); edge=data["edges"][0]
assert edge["source_record_index"] == edge["target_record_index"] == 0
assert edge["resolution"] == "resolved"
PY

if FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
     --emit-semantic-project-graph-json "$tmpdir/combined.json" -c "$controller_source" \
     >"$tmpdir/combined.stdout" 2>"$tmpdir/combined.stderr"; then
  echo "ERROR: combined graph/compiler mode unexpectedly succeeded" >&2
  exit 1
fi
grep -q "cannot be combined" "$tmpdir/combined.stderr"

if FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
     >"$tmpdir/unpaired.stdout" 2>"$tmpdir/unpaired.stderr"; then
  echo "ERROR: unpaired graph option unexpectedly succeeded" >&2
  exit 1
fi
grep -q "requires both paired options" "$tmpdir/unpaired.stderr"

cp "$tmpdir/controller.json" "$tmpdir/controller.protected.json"
if FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
     --emit-semantic-project-graph-json "$tmpdir/controller.json" \
     >"$tmpdir/conflict.stdout" 2>"$tmpdir/conflict.stderr"; then
  echo "ERROR: graph output/input collision unexpectedly succeeded" >&2
  exit 1
fi
grep -q "output conflicts with an input or source path" "$tmpdir/conflict.stderr"
cmp "$tmpdir/controller.json" "$tmpdir/controller.protected.json"

mkdir "$tmpdir/output-directory"
if FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/manifest.json" \
     --emit-semantic-project-graph-json "$tmpdir/output-directory" \
     >"$tmpdir/atomic.stdout" 2>"$tmpdir/atomic.stderr"; then
  echo "ERROR: project graph replacement over directory unexpectedly succeeded" >&2
  exit 1
fi
test -d "$tmpdir/output-directory"

FISICS_MAX_PROCS=0 "$BIN" --help >"$tmpdir/help.txt"
grep -q -- '--semantic-project-manifest' "$tmpdir/help.txt"
grep -q -- '--emit-semantic-project-graph-json' "$tmpdir/help.txt"

if find "$tmpdir" -name '*.tmp.*' -print -quit | grep -q .; then
  echo "ERROR: semantic project graph temporary file was not cleaned up" >&2
  exit 1
fi

echo "Semantic project graph contract passed"
