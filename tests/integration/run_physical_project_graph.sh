#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="${1:-./fisics}"
cd "$ROOT"

tmpdir="$(mktemp -d "${TMPDIR:-/tmp}/fisics-physical-project.XXXXXX")"
trap 'if [ "${FISICS_KEEP_TMP:-0}" = 1 ]; then echo "kept $tmpdir" >&2; else rm -rf "$tmpdir"; fi' EXIT

for stem in controller physical_values; do
  source="tests/fixtures/semantic_metadata_sm7/$stem.c"
  FISICS_MAX_PROCS=0 "$BIN" --overlay=physics-units,semantic-metadata \
    --emit-semantic-metadata-json "$tmpdir/$stem.semantic.json" \
    --emit-units-metadata-json "$tmpdir/$stem.units.json" \
    -c "$source" -o "$tmpdir/$stem.o" >/dev/null
done

python3 - "$tmpdir" <<'PY'
import json, pathlib, sys
d = pathlib.Path(sys.argv[1])
(d / "semantic.manifest.json").write_text(json.dumps({
    "schema": "fisiCs.semantic_project_manifest", "schema_version": 1,
    "project_id": "van.control",
    "translation_units": [
        {"metadata_path": "controller.semantic.json"},
        {"metadata_path": "physical_values.semantic.json"},
    ],
    "allowed_external_targets": ["device.cabin.fan.command", "frame.van.body"],
}, indent=2) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --semantic-project-manifest "$tmpdir/semantic.manifest.json" \
  --emit-semantic-project-graph-json "$tmpdir/semantic.graph.json"

python3 - "$tmpdir" <<'PY'
import json, pathlib, sys
d = pathlib.Path(sys.argv[1])
semantic = json.loads((d / "semantic.graph.json").read_text())
fallback = next(r["source_identity"] for r in semantic["records"]
                if r["symbol_name"] == "fallback_distance")

def quantity(id, vector, text):
    return {"id": id, "dimension_system": "fisiCs.si_dim8.v1",
            "dimension_vector": vector, "dimension_text": text}

def contract(id, subject, quantity_kind, mode="dimension_only", unit=None,
             form="scalar", components=None, reference="linear", frame=None):
    return {
        "contract_id": id,
        "subject": {"kind": subject[0], "id": subject[1]},
        "quantity_kind": quantity_kind,
        "unit_policy": {"mode": mode, "unit": unit},
        "value_form": {"kind": form, "component_count": components,
                       "tensor_rank": None, "tensor_shape": []},
        "reference_semantics": {"kind": reference, "reference_subject": None,
                                "period_binary64": None},
        "frame_id": frame,
    }

S = "semantic_id"
catalog = {
    "schema": "fisiCs.physical_contract_catalog", "schema_version": 1,
    "catalog_id": "van.physical.contracts",
    "quantity_kinds": [
        quantity("quantity.temperature.absolute", [0,0,0,0,1,0,0,0], "temperature"),
        quantity("quantity.temperature.difference", [0,0,0,0,1,0,0,0], "temperature"),
        quantity("quantity.velocity", [1,0,-1,0,0,0,0,0], "velocity"),
        quantity("quantity.energy", [2,1,-2,0,0,0,0,0], "energy"),
        quantity("quantity.torque", [2,1,-2,0,0,0,0,0], "energy"),
        quantity("quantity.length", [1,0,0,0,0,0,0,0], "length"),
        quantity("quantity.dimensionless", [0,0,0,0,0,0,0,0], "dimensionless"),
    ],
    "contracts": [
        contract("contract.temperature", (S, "signal.cabin.temperature"),
                 "quantity.temperature.absolute", "exact_unit", "kelvin", reference="absolute"),
        contract("contract.temperature_delta", (S, "signal.cabin.temperature_delta"),
                 "quantity.temperature.difference", "exact_unit", "kelvin", reference="difference"),
        contract("contract.velocity", (S, "signal.cabin.velocity"), "quantity.velocity",
                 "convertible_unit", "meter_per_second", "vector", 3, frame="frame.van.body"),
        contract("contract.energy", (S, "quantity.cabin.energy"), "quantity.energy",
                 "exact_unit", "joule"),
        contract("contract.torque", (S, "quantity.cabin.torque"), "quantity.torque"),
        contract("contract.fallback", ("source_identity", fallback), "quantity.length",
                 "convertible_unit", "meter"),
        contract("contract.external_fan", (S, "device.cabin.fan.command"),
                 "quantity.temperature.difference"),
        contract("contract.uninstrumented", (S, "signal.cabin.uninstrumented"),
                 "quantity.dimensionless"),
        contract("contract.self_observe", (S, "method.cabin.self_observe"),
                 "quantity.dimensionless", form="opaque"),
    ],
}
(d / "catalog.json").write_text(json.dumps(catalog, indent=2) + "\n")
(d / "shared.catalog.json").write_text(json.dumps({
    "schema": "fisiCs.physical_contract_catalog", "schema_version": 1,
    "catalog_id": "van.physical.shared",
    "quantity_kinds": [catalog["quantity_kinds"][0]],
    "contracts": [],
}, indent=2) + "\n")
(d / "physical.manifest.json").write_text(json.dumps({
    "schema": "fisiCs.physical_project_manifest", "schema_version": 1,
    "project_id": "van.control",
    "semantic_project_graph_path": "semantic.graph.json",
    "units_metadata_paths": ["controller.units.json", "physical_values.units.json"],
    "physical_catalog_paths": ["catalog.json", "shared.catalog.json"],
    "coverage_policy": "require_relationship_targets",
    "source_evidence_policy": "report_unverified",
}, indent=2) + "\n")
PY

FISICS_MAX_PROCS=0 "$BIN" --physical-project-manifest "$tmpdir/physical.manifest.json" \
  --emit-physical-project-graph-json "$tmpdir/one.json"
FISICS_MAX_PROCS=0 "$BIN" --emit-physical-project-graph-json "$tmpdir/two.json" \
  --physical-project-manifest "$tmpdir/physical.manifest.json"
cmp "$tmpdir/one.json" "$tmpdir/two.json"

python3 - "$tmpdir/one.json" "$tmpdir" "$ROOT" <<'PY'
import hashlib, json, pathlib, sys
path = pathlib.Path(sys.argv[1]); raw = path.read_bytes(); data = json.loads(raw)
if not raw.endswith(b"\n") or raw.endswith(b"\n\n"):
    raise SystemExit("physical graph must have exactly one final newline")
if list(data) != ["schema", "schema_version", "producer", "project", "complete",
                  "physical_acceptance", "dimension_system", "unit_registry", "coverage",
                  "semantic_graph_receipt", "source_receipts", "quantity_kinds",
                  "units_bindings", "physical_contracts", "semantic_edges",
                  "unbound_units_evidence"]:
    raise SystemExit("unexpected physical graph key order")
if data["schema"] != "fisiCs.physical_project_graph" or not data["complete"]:
    raise SystemExit("unexpected physical graph identity")
if data["physical_acceptance"] != "not_assessed":
    raise SystemExit("SM-7 must not claim physical acceptance")
contracts = {c["contract_id"]: c for c in data["physical_contracts"]}
for name in ("contract.temperature", "contract.temperature_delta", "contract.velocity",
             "contract.energy", "contract.torque", "contract.fallback"):
    if contracts[name]["state"] != "source_compatible":
        raise SystemExit(f"{name} is not source-compatible")
if contracts["contract.external_fan"]["state"] != "external_declared":
    raise SystemExit("external contract gained source verification")
for name in ("contract.uninstrumented", "contract.self_observe"):
    if contracts[name]["state"] != "source_unverified" or contracts[name]["evidence_origin"] != "none":
        raise SystemExit(f"{name} did not retain absent source evidence")
if contracts["contract.temperature"]["reference_semantics"]["kind"] != "absolute" or \
   contracts["contract.temperature_delta"]["reference_semantics"]["kind"] != "difference":
    raise SystemExit("temperature reference semantics were collapsed")
velocity = contracts["contract.velocity"]
if velocity["frame_id"] != "frame.van.body" or velocity["value_form"]["kind"] != "vector" or \
   velocity["value_form"]["component_count"] != 3:
    raise SystemExit("vector/frame facets were not retained")
if velocity["conversion"] != {"model": "linear", "scale_binary64": "0x1.381d7dbf487fdp-2",
                              "offset_binary64": "0x0p+0"}:
    raise SystemExit(f"unexpected velocity conversion: {velocity['conversion']}")
if contracts["contract.fallback"]["subject"]["kind"] != "source_identity":
    raise SystemExit("fallback source identity was not retained")
quantities = {q["id"]: q for q in data["quantity_kinds"]}
if quantities["quantity.energy"]["dimension_vector"] != quantities["quantity.torque"]["dimension_vector"]:
    raise SystemExit("energy/torque fixture no longer tests shared dimensions")
if quantities["quantity.energy"]["id"] == quantities["quantity.torque"]["id"]:
    raise SystemExit("energy and torque quantity identity collapsed")
unbound = [data["units_bindings"][i]["symbol_name"] for i in data["unbound_units_evidence"]]
if unbound != ["internal_duration"]:
    raise SystemExit(f"unexpected unbound units evidence: {unbound}")
if data["coverage"] != {"uncovered_authored_records": 1,
                       "uncovered_relationship_targets": 0,
                       "source_unverified_contracts": 2,
                       "external_declared_contracts": 1}:
    raise SystemExit(f"unexpected coverage summary: {data['coverage']}")
if any(edge["target_contract_index"] is None for edge in data["semantic_edges"]):
    raise SystemExit("strict relationship coverage left an edge uncovered")
self_edges = [edge for edge in data["semantic_edges"]
              if edge["target_id"] == "method.cabin.self_observe"]
if len(self_edges) != 1:
    raise SystemExit("semantic self-edge was not retained exactly once")
normalized = raw.decode().replace(str(pathlib.Path(sys.argv[2]).resolve()), "@TMP@")
normalized = normalized.replace(str(pathlib.Path(sys.argv[3]).resolve()), "@ROOT@")
normalized = normalized.replace(data["producer"]["version"], "@VERSION@")
actual_digest = hashlib.sha256(normalized.encode()).hexdigest()
expected_digest = "4184ff9770aaaaf9ebef46da52dcba2b8a3451411462cd589e565bc2aa7e4f18"
if actual_digest != expected_digest:
    raise SystemExit(f"physical graph differs from frozen normalized-byte golden: {actual_digest}")
PY

cp "$tmpdir/catalog.json" "$tmpdir/catalog.original.json"
cp "$tmpdir/physical.manifest.json" "$tmpdir/physical.manifest.original.json"
python3 - "$tmpdir" <<'PY'
import json, pathlib, sys
d = pathlib.Path(sys.argv[1])
catalog = json.loads((d / "catalog.json").read_text())
catalog["quantity_kinds"].reverse(); catalog["contracts"].reverse()
(d / "catalog.json").write_text(json.dumps(catalog, separators=(",", ":")) + "\n")
manifest = json.loads((d / "physical.manifest.json").read_text())
manifest["units_metadata_paths"].reverse(); manifest["physical_catalog_paths"].reverse()
(d / "physical.manifest.json").write_text(json.dumps(manifest, separators=(",", ":")) + "\n")
PY
FISICS_MAX_PROCS=0 "$BIN" --physical-project-manifest "$tmpdir/physical.manifest.json" \
  --emit-physical-project-graph-json "$tmpdir/permuted.json"
diff -u "$tmpdir/one.json" "$tmpdir/permuted.json"
cp "$tmpdir/catalog.original.json" "$tmpdir/catalog.json"
cp "$tmpdir/physical.manifest.original.json" "$tmpdir/physical.manifest.json"

expect_failure_preserves() {
  local manifest="$1" needle="$2" label="$3"
  printf 'preserve-%s\n' "$label" >"$tmpdir/$label.output"
  if FISICS_MAX_PROCS=0 "$BIN" --physical-project-manifest "$manifest" \
       --emit-physical-project-graph-json "$tmpdir/$label.output" \
       >"$tmpdir/$label.stdout" 2>"$tmpdir/$label.stderr"; then
    echo "ERROR: $label unexpectedly succeeded" >&2
    exit 1
  fi
  grep -q "$needle" "$tmpdir/$label.stderr"
  grep -qx "preserve-$label" "$tmpdir/$label.output"
}

printf 'preserve-allocation\n' >"$tmpdir/allocation.output"
if FISICS_TEST_FAIL_PHYSICAL_PROJECT_GRAPH_ALLOC=1 FISICS_MAX_PROCS=0 "$BIN" \
     --physical-project-manifest "$tmpdir/physical.manifest.json" \
     --emit-physical-project-graph-json "$tmpdir/allocation.output" \
     >"$tmpdir/allocation.stdout" 2>"$tmpdir/allocation.stderr"; then
  echo "ERROR: allocation injection unexpectedly succeeded" >&2; exit 1
fi
grep -q 'injected physical project graph allocation failure' "$tmpdir/allocation.stderr"
grep -qx 'preserve-allocation' "$tmpdir/allocation.output"

printf 'preserve-write\n' >"$tmpdir/write.output"
if FISICS_TEST_FAIL_PHYSICAL_PROJECT_GRAPH_WRITE=1 FISICS_MAX_PROCS=0 "$BIN" \
     --physical-project-manifest "$tmpdir/physical.manifest.json" \
     --emit-physical-project-graph-json "$tmpdir/write.output" \
     >"$tmpdir/write.stdout" 2>"$tmpdir/write.stderr"; then
  echo "ERROR: write injection unexpectedly succeeded" >&2; exit 1
fi
grep -q 'failed to atomically publish physical project graph' "$tmpdir/write.stderr"
grep -qx 'preserve-write' "$tmpdir/write.output"

python3 - "$tmpdir" <<'PY'
import json, pathlib, sys
d = pathlib.Path(sys.argv[1])

def mutate(source, output, change):
    data = json.loads((d / source).read_text()); change(data)
    (d / output).write_text(json.dumps(data) + "\n")

mutate("catalog.json", "dimension-mismatch.catalog.json",
       lambda x: next(q for q in x["quantity_kinds"] if q["id"] == "quantity.velocity").update(dimension_vector=[1,0,0,0,0,0,0,0], dimension_text="length"))
mutate("physical.manifest.json", "dimension-mismatch.manifest.json",
       lambda x: x.update(physical_catalog_paths=["dimension-mismatch.catalog.json", "shared.catalog.json"]))
mutate("catalog.json", "missing-target.catalog.json",
       lambda x: x.update(contracts=[c for c in x["contracts"] if c["contract_id"] != "contract.external_fan"]))
mutate("physical.manifest.json", "missing-target.manifest.json",
       lambda x: x.update(physical_catalog_paths=["missing-target.catalog.json", "shared.catalog.json"]))
mutate("physical_values.units.json", "fingerprint.units.json",
       lambda x: x["unit_registry"].update(fingerprint_fnv1a64="0000000000000000"))
mutate("physical.manifest.json", "fingerprint.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json", "fingerprint.units.json"]))
mutate("physical.manifest.json", "missing-receipt.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json"]))
mutate("physical.manifest.json", "duplicate-receipt.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json", "controller.units.json"]))
mutate("catalog.json", "duplicate-subject.catalog.json",
       lambda x: x["contracts"].append(dict(x["contracts"][0], contract_id="contract.temperature.duplicate")))
mutate("physical.manifest.json", "duplicate-subject.manifest.json",
       lambda x: x.update(physical_catalog_paths=["duplicate-subject.catalog.json", "shared.catalog.json"]))
mutate("physical.manifest.json", "require-source.manifest.json",
       lambda x: x.update(source_evidence_policy="require_for_source_contracts"))
mutate("catalog.json", "exact-unit.catalog.json",
       lambda x: next(c for c in x["contracts"] if c["contract_id"] == "contract.energy")["unit_policy"].update(unit="kilojoule"))
mutate("physical.manifest.json", "exact-unit.manifest.json",
       lambda x: x.update(physical_catalog_paths=["exact-unit.catalog.json", "shared.catalog.json"]))
mutate("catalog.json", "unknown-unit.catalog.json",
       lambda x: next(c for c in x["contracts"] if c["contract_id"] == "contract.energy")["unit_policy"].update(unit="unknown_unit"))
mutate("physical.manifest.json", "unknown-unit.manifest.json",
       lambda x: x.update(physical_catalog_paths=["unknown-unit.catalog.json", "shared.catalog.json"]))
mutate("physical_values.units.json", "stale.units.json",
       lambda x: x["source"].update(hash_fnv1a64="0000000000000000"))
mutate("physical.manifest.json", "stale.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json", "stale.units.json"]))
mutate("controller.units.json", "duplicate-source.units.json", lambda x: None)
mutate("physical.manifest.json", "duplicate-source.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json", "duplicate-source.units.json"]))
mutate("physical.manifest.json", "extra-receipt.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json", "physical_values.units.json", "duplicate-source.units.json"]))
mutate("physical_values.units.json", "incomplete.units.json", lambda x: x.update(complete=False))
mutate("physical.manifest.json", "incomplete.manifest.json",
       lambda x: x.update(units_metadata_paths=["controller.units.json", "incomplete.units.json"]))
mutate("catalog.json", "bad-frame.catalog.json",
       lambda x: next(c for c in x["contracts"] if c["contract_id"] == "contract.velocity").update(frame_id="frame.missing"))
mutate("physical.manifest.json", "bad-frame.manifest.json",
       lambda x: x.update(physical_catalog_paths=["bad-frame.catalog.json", "shared.catalog.json"]))
mutate("catalog.json", "dimension-text.catalog.json",
       lambda x: next(q for q in x["quantity_kinds"] if q["id"] == "quantity.velocity").update(dimension_text="length"))
mutate("physical.manifest.json", "dimension-text.manifest.json",
       lambda x: x.update(physical_catalog_paths=["dimension-text.catalog.json", "shared.catalog.json"]))

base = json.loads((d / "catalog.json").read_text())
conflict = {"schema": "fisiCs.physical_contract_catalog", "schema_version": 1,
            "catalog_id": "van.physical.conflict",
            "quantity_kinds": [dict(base["quantity_kinds"][0],
                                    dimension_vector=[1,0,0,0,0,0,0,0], dimension_text="length")],
            "contracts": []}
(d / "conflict.catalog.json").write_text(json.dumps(conflict) + "\n")
mutate("physical.manifest.json", "conflict.manifest.json",
       lambda x: x.update(physical_catalog_paths=["catalog.json", "conflict.catalog.json"]))
PY
expect_failure_preserves "$tmpdir/dimension-mismatch.manifest.json" 'physical dimension mismatch' dimension-mismatch
expect_failure_preserves "$tmpdir/missing-target.manifest.json" 'missing required physical contract' missing-target
expect_failure_preserves "$tmpdir/fingerprint.manifest.json" 'units registry fingerprint mismatch' fingerprint
expect_failure_preserves "$tmpdir/missing-receipt.manifest.json" 'exactly one units receipt' missing-receipt
expect_failure_preserves "$tmpdir/duplicate-receipt.manifest.json" 'duplicate units_metadata_paths path' duplicate-receipt
expect_failure_preserves "$tmpdir/duplicate-subject.manifest.json" 'duplicate physical contract subject' duplicate-subject
expect_failure_preserves "$tmpdir/require-source.manifest.json" 'lacks required source units evidence' require-source
expect_failure_preserves "$tmpdir/exact-unit.manifest.json" 'physical exact-unit mismatch' exact-unit
expect_failure_preserves "$tmpdir/unknown-unit.manifest.json" 'expected unit is unknown' unknown-unit
expect_failure_preserves "$tmpdir/stale.manifest.json" 'stale physical units source hash' stale
expect_failure_preserves "$tmpdir/duplicate-source.manifest.json" 'duplicate units source receipt' duplicate-source
expect_failure_preserves "$tmpdir/extra-receipt.manifest.json" 'exactly one units receipt' extra-receipt
expect_failure_preserves "$tmpdir/incomplete.manifest.json" 'requires complete enabled units metadata' incomplete
expect_failure_preserves "$tmpdir/bad-frame.manifest.json" 'references missing frame' bad-frame
expect_failure_preserves "$tmpdir/dimension-text.manifest.json" 'dimension_text disagrees' dimension-text
expect_failure_preserves "$tmpdir/conflict.manifest.json" 'conflicting quantity kind' conflict

if FISICS_MAX_PROCS=0 "$BIN" --physical-project-manifest "$tmpdir/physical.manifest.json" \
     --emit-physical-project-graph-json "$tmpdir/physical.manifest.json" \
     >"$tmpdir/alias.stdout" 2>"$tmpdir/alias.stderr"; then
  echo "ERROR: output alias unexpectedly succeeded" >&2; exit 1
fi
grep -q 'conflicts with an input or source path' "$tmpdir/alias.stderr"

if FISICS_MAX_PROCS=0 "$BIN" --physical-project-manifest "$tmpdir/physical.manifest.json" \
     --emit-physical-project-graph-json "$tmpdir/mixed.json" -c tests/fixtures/semantic_metadata_sm7/controller.c \
     >"$tmpdir/mixed.stdout" 2>"$tmpdir/mixed.stderr"; then
  echo "ERROR: mixed physical/compile CLI unexpectedly succeeded" >&2; exit 1
fi
grep -q 'requires both paired options exactly once' "$tmpdir/mixed.stderr"

if find "$tmpdir" -name '*.tmp.*' -print -quit | grep -q .; then
  echo "ERROR: physical graph temporary file leaked" >&2; exit 1
fi

echo "physical project graph integration passed"
