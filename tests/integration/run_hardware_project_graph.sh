#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-./fisics}"
cd "$ROOT"

tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/fisics-hardware-project.XXXXXX")"
sm7_tmp=""
cleanup() {
  if [ "${FISICS_KEEP_TMP:-0}" = 1 ]; then
    echo "kept $tmpdir" >&2
    [ -z "$sm7_tmp" ] || echo "kept $sm7_tmp" >&2
  else
    rm -rf "$tmpdir"
    [ -z "$sm7_tmp" ] || rm -rf "$sm7_tmp"
  fi
}
trap cleanup EXIT

FISICS_KEEP_TMP=1 bash tests/integration/run_physical_project_graph.sh "$BIN" \
  >"$tmpdir/sm7.stdout" 2>"$tmpdir/sm7.stderr"
sm7_tmp="$(sed -n 's/^kept //p' "$tmpdir/sm7.stderr" | tail -1)"
test -n "$sm7_tmp"
test -f "$sm7_tmp/one.json"

cp tests/fixtures/semantic_metadata_sm8/project.hardware.json "$tmpdir/project.hardware.json"
cp tests/fixtures/semantic_metadata_sm8/vendor.hardware.json "$tmpdir/vendor.hardware.json"
cp tests/fixtures/semantic_metadata_sm8/hardware.topology.json "$tmpdir/hardware.topology.json"

python3 - "$tmpdir" "$sm7_tmp/one.json" <<'PY'
import json, pathlib, sys
d = pathlib.Path(sys.argv[1])
(d / "hardware.manifest.json").write_text(json.dumps({
    "schema": "fisiCs.hardware_project_manifest",
    "schema_version": 1,
    "project_id": "van.control",
    "physical_project_graph_path": sys.argv[2],
    "hardware_catalog_paths": ["project.hardware.json", "vendor.hardware.json"],
    "hardware_topology_path": "hardware.topology.json",
    "physical_evidence_policy": "require_source_compatible",
    "required_port_policy": "require_all",
}, indent=2) + "\n")
PY

FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
  --emit-hardware-project-graph-json "$tmpdir/one.json"
FISICS_MAX_PROCS=0 "$BIN" --emit-hardware-project-graph-json "$tmpdir/two.json" \
  --hardware-project-manifest "$tmpdir/hardware.manifest.json"
cmp "$tmpdir/one.json" "$tmpdir/two.json"

python3 - "$tmpdir/one.json" "$tmpdir" "$ROOT" "$sm7_tmp" <<'PY'
import hashlib, json, pathlib, sys
path = pathlib.Path(sys.argv[1])
raw = path.read_bytes()
data = json.loads(raw)
expected_keys = [
    "schema", "schema_version", "producer", "project", "complete",
    "assessment", "coverage", "physical_graph_receipt", "input_receipts",
    "catalogs", "bus_profiles", "board_profiles", "device_models",
    "driver_contracts", "nodes", "buses", "devices", "connections",
    "signal_bindings", "unbound_required_ports",
]
if list(data) != expected_keys or data["schema"] != "fisiCs.hardware_project_graph" or not data["complete"]:
    raise SystemExit("unexpected hardware graph root contract")
assessment = data["assessment"]
if assessment["catalog_integrity"] != "validated" or assessment["declared_topology"] != "compatible":
    raise SystemExit("hardware declaration assessment did not pass")
for key in ("hardware_presence", "wiring", "driver_implementation", "calibration",
            "runtime_observation", "simulation", "deployment", "physical_acceptance"):
    if assessment[key] != "not_assessed":
        raise SystemExit(f"SM-8 improperly promoted {key}")
if len(data["nodes"]) != 1 or len(data["buses"]) != 1 or len(data["devices"]) != 2:
    raise SystemExit("frozen node/bus/sensor/actuator canary changed")
catalogs = {c["catalog_id"]: c for c in data["catalogs"]}
if catalogs["catalog.vendor.example"]["authority"]["kind"] != "external_declared":
    raise SystemExit("external catalog authority was promoted")
models = {m["model_id"]: m for m in data["device_models"]}
if "device.sensor" not in models["device.environment.sensor"]["capability_ids"]:
    raise SystemExit("sensor capability missing")
if "device.actuator" not in models["device.fan.controller"]["capability_ids"]:
    raise SystemExit("actuator capability missing")
bindings = {b["binding_id"]: b for b in data["signal_bindings"]}
if bindings["binding.temperature.measurement"]["compatibility_mode"] != "exact_quantity":
    raise SystemExit("exact-quantity bridge changed")
if bindings["binding.fan.command"]["compatibility_mode"] != "dimension_only":
    raise SystemExit("dimension-only bridge changed")
if any(b["physical_evidence_state"] != "source_compatible" or b["result"] != "compatible"
       for b in bindings.values()):
    raise SystemExit("SM-7 evidence state was not retained")
if data["coverage"] != {"unbound_required_ports": 0} or data["unbound_required_ports"]:
    raise SystemExit("positive canary has unbound required ports")
if not raw.endswith(b"\n") or raw.endswith(b"\n\n"):
    raise SystemExit("hardware graph final newline contract changed")
# The parent byte length changes with its embedded absolute source identities.
# Independently verify the full parent receipt before normalizing hash and size.
parent_bytes = (pathlib.Path(sys.argv[4]) / "one.json").read_bytes()
parent_hash = 1469598103934665603
for byte in parent_bytes:
    parent_hash = ((parent_hash ^ byte) * 1099511628211) & ((1 << 64) - 1)
receipt = data["physical_graph_receipt"]
if (receipt["length_bytes"] != len(parent_bytes) or
    receipt["hash_fnv1a64"] != f"{parent_hash:016x}"):
    raise SystemExit("hardware graph lost its exact parent byte receipt")
normalized = raw.decode().replace(str(pathlib.Path(sys.argv[2]).resolve()), "@TMP@")
normalized = normalized.replace(str(pathlib.Path(sys.argv[3]).resolve()), "@ROOT@")
normalized = normalized.replace(str(pathlib.Path(sys.argv[4]).resolve()), "@SM7_TMP@")
normalized = normalized.replace(data["producer"]["version"], "@VERSION@")
normalized = normalized.replace(data["physical_graph_receipt"]["hash_fnv1a64"], "@PHYSICAL_HASH@")
normalized = normalized.replace('"length_bytes":' + str(receipt["length_bytes"]),
                                '"length_bytes":"@PHYSICAL_LENGTH@"', 1)
digest = hashlib.sha256(normalized.encode()).hexdigest()
expected = "b192c04dd988a92f698fb1e8b683ebcc80143a83e30f03e0a26962be33debef4"
if digest != expected:
    raise SystemExit(f"hardware graph differs from frozen normalized-byte golden: {digest}")
print("SM8_DIGEST=" + digest)
PY

cp "$tmpdir/hardware.manifest.json" "$tmpdir/hardware.manifest.original.json"
python3 - "$tmpdir/hardware.manifest.json" <<'PY'
import json, pathlib, sys
p = pathlib.Path(sys.argv[1])
data = json.loads(p.read_text())
data["hardware_catalog_paths"].reverse()
p.write_text(json.dumps(data, separators=(",", ":")) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
  --emit-hardware-project-graph-json "$tmpdir/permuted.json"
cmp "$tmpdir/one.json" "$tmpdir/permuted.json"
cp "$tmpdir/hardware.manifest.original.json" "$tmpdir/hardware.manifest.json"

expect_failure_preserves() {
  local manifest="$1" needle="$2" label="$3"
  printf 'preserve-%s\n' "$label" >"$tmpdir/$label.output"
  if FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$manifest" \
       --emit-hardware-project-graph-json "$tmpdir/$label.output" \
       >"$tmpdir/$label.stdout" 2>"$tmpdir/$label.stderr"; then
    echo "ERROR: $label unexpectedly succeeded" >&2
    exit 1
  fi
  grep -q "$needle" "$tmpdir/$label.stderr"
  grep -qx "preserve-$label" "$tmpdir/$label.output"
}

mutate_topology() {
  local output="$1" expression="$2"
  python3 - "$tmpdir/hardware.topology.json" "$output" "$expression" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
exec(sys.argv[3], {"data": data})
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
  python3 - "$tmpdir/hardware.manifest.json" "$output" "$output.manifest" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["hardware_topology_path"] = pathlib.Path(sys.argv[2]).name
pathlib.Path(sys.argv[3]).write_text(json.dumps(data, indent=2) + "\n")
PY
}

mutate_topology "$tmpdir/collision.topology.json" \
  'data["connections"][1]["allocation_claims"][0]["value_uint64"]="47"'
expect_failure_preserves "$tmpdir/collision.topology.json.manifest" \
  'exclusive allocation conflict' allocation_collision

mutate_topology "$tmpdir/missing-model.topology.json" \
  'data["devices"][0]["model"]["revision"]="missing"'
expect_failure_preserves "$tmpdir/missing-model.topology.json.manifest" \
  'unresolved owner or model' missing_model

mutate_topology "$tmpdir/missing-driver.topology.json" \
  'data["devices"][0]["driver"]["driver_id"]="driver.missing"'
expect_failure_preserves "$tmpdir/missing-driver.topology.json.manifest" \
  'unresolved driver contract' missing_driver

mutate_topology "$tmpdir/missing-contract.topology.json" \
  'data["signal_bindings"][0]["physical_contract_id"]="contract.missing"'
expect_failure_preserves "$tmpdir/missing-contract.topology.json.manifest" \
  'missing physical contract' missing_contract

mutate_topology "$tmpdir/dimension.topology.json" \
  'data["signal_bindings"][0]["physical_contract_id"]="contract.velocity"'
expect_failure_preserves "$tmpdir/dimension.topology.json.manifest" \
  'dimension mismatch' dimension_mismatch

mutate_topology "$tmpdir/unbound.topology.json" \
  'data["signal_bindings"].pop()'
expect_failure_preserves "$tmpdir/unbound.topology.json.manifest" \
  'unbound required physical port' unbound_required

python3 - "$tmpdir/unbound.topology.json.manifest" "$tmpdir/report.manifest.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["required_port_policy"] = "report_unbound"
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/report.manifest.json" \
  --emit-hardware-project-graph-json "$tmpdir/report.json"
python3 - "$tmpdir/report.json" <<'PY'
import json, sys
data = json.load(open(sys.argv[1]))
assert data["assessment"]["declared_topology"] == "incomplete"
assert data["coverage"]["unbound_required_ports"] == 1
PY

cp "$tmpdir/vendor.hardware.json" "$tmpdir/vendor.original.json"
python3 - "$tmpdir/vendor.hardware.json" <<'PY'
import json, pathlib, sys
p = pathlib.Path(sys.argv[1])
data = json.loads(p.read_text())
data["driver_contracts"][0]["supported_models"][0]["revision"] = "wrong"
p.write_text(json.dumps(data, indent=2) + "\n")
PY
expect_failure_preserves "$tmpdir/hardware.manifest.json" \
  'does not support device model' unsupported_driver
cp "$tmpdir/vendor.original.json" "$tmpdir/vendor.hardware.json"

python3 - "$tmpdir/project.hardware.json" "$tmpdir/duplicate.hardware.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["catalog_id"] = "catalog.van.duplicate"
data["authority"] = {"kind": "external_declared", "identity": "authority.van.duplicate"}
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
python3 - "$tmpdir/hardware.manifest.json" "$tmpdir/duplicate.manifest.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["hardware_catalog_paths"].append("duplicate.hardware.json")
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/duplicate.manifest.json" \
  --emit-hardware-project-graph-json "$tmpdir/duplicate.json"
python3 - "$tmpdir/duplicate.json" <<'PY'
import json, sys
data = json.load(open(sys.argv[1]))
assert len(data["bus_profiles"]) == 1 and len(data["board_profiles"]) == 1
assert len(data["bus_profiles"][0]["origin_catalogs"]) == 2
assert len(data["board_profiles"][0]["origin_catalogs"]) == 2
assert len(data["catalogs"]) == 3
PY

python3 - "$tmpdir/duplicate.hardware.json" <<'PY'
import json, pathlib, sys
p = pathlib.Path(sys.argv[1]); data = json.loads(p.read_text())
data["bus_profiles"][0]["allocation_spaces"][0]["maximum_uint64"] = "126"
p.write_text(json.dumps(data, indent=2) + "\n")
PY
expect_failure_preserves "$tmpdir/duplicate.manifest.json" \
  'conflicting hardware bus profile' conflicting_definition

python3 - "$tmpdir/hardware.topology.json" "$tmpdir/evidence.topology.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["signal_bindings"][0]["physical_contract_id"] = "contract.external_fan"
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
python3 - "$tmpdir/vendor.hardware.json" "$tmpdir/evidence.vendor.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
for model in data["device_models"]:
    if model["model_id"] == "device.fan.controller":
        model["physical_ports"][0]["requirement"]["reference_kind"] = None
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
python3 - "$tmpdir/hardware.manifest.json" "$tmpdir/evidence.topology.json" \
  "$tmpdir/evidence.vendor.json" "$tmpdir/evidence-require.manifest.json" \
  "$tmpdir/evidence-allow.manifest.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["hardware_topology_path"] = pathlib.Path(sys.argv[2]).name
data["hardware_catalog_paths"] = ["project.hardware.json", pathlib.Path(sys.argv[3]).name]
pathlib.Path(sys.argv[4]).write_text(json.dumps(data, indent=2) + "\n")
data["physical_evidence_policy"] = "allow_declared"
pathlib.Path(sys.argv[5]).write_text(json.dumps(data, indent=2) + "\n")
PY
expect_failure_preserves "$tmpdir/evidence-require.manifest.json" \
  'requires source_compatible physical evidence' evidence_policy
FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/evidence-allow.manifest.json" \
  --emit-hardware-project-graph-json "$tmpdir/evidence-allow.json"
python3 - "$tmpdir/evidence-allow.json" <<'PY'
import json,sys
data=json.load(open(sys.argv[1])); bindings={b["binding_id"]:b for b in data["signal_bindings"]}
assert bindings["binding.fan.command"]["physical_evidence_state"] == "external_declared"
PY

python3 - "$sm7_tmp/one.json" "$tmpdir/stale.physical.json" <<'PY'
import json, pathlib, sys
data = json.loads(pathlib.Path(sys.argv[1]).read_text())
data["source_receipts"][0]["source"]["length_bytes"] += 1
pathlib.Path(sys.argv[2]).write_text(json.dumps(data, indent=2) + "\n")
PY
python3 - "$tmpdir/hardware.manifest.json" "$tmpdir/stale.manifest.json" <<'PY'
import json,pathlib,sys
data=json.loads(pathlib.Path(sys.argv[1]).read_text())
data["physical_project_graph_path"]="stale.physical.json"
pathlib.Path(sys.argv[2]).write_text(json.dumps(data,indent=2)+"\n")
PY
expect_failure_preserves "$tmpdir/stale.manifest.json" \
  'stale hardware input physical source length' stale_source

python3 - "$tmpdir/project.hardware.json" "$tmpdir/unknown.hardware.json" <<'PY'
import json,pathlib,sys
data=json.loads(pathlib.Path(sys.argv[1]).read_text());data["unknown_field"]=True
pathlib.Path(sys.argv[2]).write_text(json.dumps(data,indent=2)+"\n")
PY
python3 - "$tmpdir/hardware.manifest.json" "$tmpdir/unknown.manifest.json" <<'PY'
import json,pathlib,sys
data=json.loads(pathlib.Path(sys.argv[1]).read_text())
data["hardware_catalog_paths"]=["unknown.hardware.json","vendor.hardware.json"]
pathlib.Path(sys.argv[2]).write_text(json.dumps(data,indent=2)+"\n")
PY
expect_failure_preserves "$tmpdir/unknown.manifest.json" \
  'invalid hardware component catalog header' unknown_field

python3 - "$tmpdir/hardware.manifest.json" "$tmpdir/input-alias.manifest.json" <<'PY'
import json,pathlib,sys
data=json.loads(pathlib.Path(sys.argv[1]).read_text())
data["hardware_topology_path"]=pathlib.Path(sys.argv[2]).name
pathlib.Path(sys.argv[2]).write_text(json.dumps(data,indent=2)+"\n")
PY
expect_failure_preserves "$tmpdir/input-alias.manifest.json" \
  'hardware input path aliases manifest input' input_alias

printf 'preserve-allocation\n' >"$tmpdir/allocation.output"
if FISICS_TEST_FAIL_HARDWARE_PROJECT_GRAPH_ALLOC=1 FISICS_MAX_PROCS=0 "$BIN" \
     --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --emit-hardware-project-graph-json "$tmpdir/allocation.output" \
     >"$tmpdir/allocation.stdout" 2>"$tmpdir/allocation.stderr"; then
  echo "ERROR: allocation injection unexpectedly succeeded" >&2
  exit 1
fi
grep -q 'injected hardware project graph allocation failure' "$tmpdir/allocation.stderr"
grep -qx 'preserve-allocation' "$tmpdir/allocation.output"

printf 'preserve-write\n' >"$tmpdir/write.output"
if FISICS_TEST_FAIL_HARDWARE_PROJECT_GRAPH_WRITE=1 FISICS_MAX_PROCS=0 "$BIN" \
     --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --emit-hardware-project-graph-json "$tmpdir/write.output" \
     >"$tmpdir/write.stdout" 2>"$tmpdir/write.stderr"; then
  echo "ERROR: write injection unexpectedly succeeded" >&2
  exit 1
fi
grep -q 'failed to atomically publish hardware project graph' "$tmpdir/write.stderr"
grep -qx 'preserve-write' "$tmpdir/write.output"

if FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     >"$tmpdir/shape.stdout" 2>"$tmpdir/shape.stderr"; then
  echo "ERROR: missing hardware output option unexpectedly succeeded" >&2; exit 1
fi
if FISICS_MAX_PROCS=0 "$BIN" --emit-hardware-project-graph-json "$tmpdir/shape.json" \
     >"$tmpdir/shape.stdout" 2>"$tmpdir/shape.stderr"; then
  echo "ERROR: missing hardware manifest option unexpectedly succeeded" >&2; exit 1
fi
if FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --emit-hardware-project-graph-json "$tmpdir/shape.json" \
     >"$tmpdir/shape.stdout" 2>"$tmpdir/shape.stderr"; then
  echo "ERROR: duplicate hardware manifest option unexpectedly succeeded" >&2; exit 1
fi
if FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --emit-hardware-project-graph-json "$tmpdir/shape.json" -c tests/fixtures/semantic_metadata_sm7/controller.c \
     >"$tmpdir/shape.stdout" 2>"$tmpdir/shape.stderr"; then
  echo "ERROR: mixed hardware compile mode unexpectedly succeeded" >&2; exit 1
fi

manifest_before="$(shasum -a 256 "$tmpdir/hardware.manifest.json")"
if FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --emit-hardware-project-graph-json "$tmpdir/hardware.manifest.json" \
     >"$tmpdir/alias.stdout" 2>"$tmpdir/alias.stderr"; then
  echo "ERROR: hardware output/input alias unexpectedly succeeded" >&2; exit 1
fi
grep -q 'conflicts with an input or source path' "$tmpdir/alias.stderr"
test "$manifest_before" = "$(shasum -a 256 "$tmpdir/hardware.manifest.json")"

source_path="tests/fixtures/semantic_metadata_sm7/controller.c"
source_before="$(shasum -a 256 "$source_path")"
if FISICS_MAX_PROCS=0 "$BIN" --hardware-project-manifest "$tmpdir/hardware.manifest.json" \
     --emit-hardware-project-graph-json "$source_path" \
     >"$tmpdir/source-alias.stdout" 2>"$tmpdir/source-alias.stderr"; then
  echo "ERROR: hardware output/source alias unexpectedly succeeded" >&2; exit 1
fi
grep -q 'conflicts with an input or source path' "$tmpdir/source-alias.stderr"
test "$source_before" = "$(shasum -a 256 "$source_path")"

if find "$tmpdir" -name '*.tmp.*' -print -quit | grep -q .; then
  echo "ERROR: hardware graph left temporary output files" >&2
  exit 1
fi

echo "hardware project graph integration passed"
