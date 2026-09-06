#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-$ROOT/fisics}"
OUT="${2:-$ROOT/build/examples/semantic_metadata/hardware_project}"
SOURCE_DIR="$ROOT/examples/semantic_metadata/hardware_project"

mkdir -p "$OUT"

for stem in controller signals; do
  FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
    -I"$ROOT/include" \
    --emit-semantic-metadata-json "$OUT/$stem.semantic.json" \
    --emit-units-metadata-json "$OUT/$stem.units.json" \
    -c "$SOURCE_DIR/$stem.c" -o "$OUT/$stem.o"
done

python3 - "$OUT" "$SOURCE_DIR" <<'PY'
import json
import pathlib
import sys

out = pathlib.Path(sys.argv[1])
source = pathlib.Path(sys.argv[2]).resolve()

(out / "semantic.manifest.json").write_text(json.dumps({
    "schema": "fisiCs.semantic_project_manifest",
    "schema_version": 1,
    "project_id": "van.control",
    "translation_units": [
        {"metadata_path": "controller.semantic.json"},
        {"metadata_path": "signals.semantic.json"},
    ],
    "allowed_external_targets": [],
}, indent=2) + "\n")

(out / "physical.manifest.json").write_text(json.dumps({
    "schema": "fisiCs.physical_project_manifest",
    "schema_version": 1,
    "project_id": "van.control",
    "semantic_project_graph_path": "semantic.graph.json",
    "units_metadata_paths": ["controller.units.json", "signals.units.json"],
    "physical_catalog_paths": [str(source / "physical.catalog.json")],
    "coverage_policy": "require_relationship_targets",
    "source_evidence_policy": "report_unverified",
}, indent=2) + "\n")

(out / "hardware.manifest.json").write_text(json.dumps({
    "schema": "fisiCs.hardware_project_manifest",
    "schema_version": 1,
    "project_id": "van.control",
    "physical_project_graph_path": "physical.graph.json",
    "hardware_catalog_paths": [
        str(source / "project.hardware.json"),
        str(source / "vendor.hardware.json"),
    ],
    "hardware_topology_path": str(source / "hardware.topology.json"),
    "physical_evidence_policy": "require_source_compatible",
    "required_port_policy": "require_all",
}, indent=2) + "\n")
PY

FISICS_MAX_PROCS=0 "$BIN" \
  --semantic-project-manifest "$OUT/semantic.manifest.json" \
  --emit-semantic-project-graph-json "$OUT/semantic.graph.json"

FISICS_MAX_PROCS=0 "$BIN" \
  --physical-project-manifest "$OUT/physical.manifest.json" \
  --emit-physical-project-graph-json "$OUT/physical.graph.json"

FISICS_MAX_PROCS=0 "$BIN" \
  --hardware-project-manifest "$OUT/hardware.manifest.json" \
  --emit-hardware-project-graph-json "$OUT/hardware.graph.json"

FISICS_MAX_PROCS=0 "$BIN" \
  --hardware-project-manifest "$OUT/hardware.manifest.json" \
  --emit-hardware-project-graph-json "$OUT/hardware.repeat.graph.json"
cmp "$OUT/hardware.graph.json" "$OUT/hardware.repeat.graph.json"

python3 - "$OUT/hardware.graph.json" <<'PY'
import json
import pathlib
import sys

data = json.loads(pathlib.Path(sys.argv[1]).read_text())
if not data["complete"]:
    raise SystemExit("hardware example graph is incomplete")
if data["assessment"]["declared_topology"] != "compatible":
    raise SystemExit("hardware example topology is not compatible")
if len(data["nodes"]) != 1 or len(data["buses"]) != 1 or len(data["devices"]) != 2:
    raise SystemExit("hardware example inventory changed unexpectedly")
if len(data["signal_bindings"]) != 2:
    raise SystemExit("hardware example expected exactly two signal bindings")
for key in ("hardware_presence", "wiring", "driver_implementation", "calibration",
            "runtime_observation", "simulation", "deployment", "physical_acceptance"):
    if data["assessment"][key] != "not_assessed":
        raise SystemExit(f"static example improperly promoted {key}")
PY

echo "semantic metadata physical-system hardware example passed"
