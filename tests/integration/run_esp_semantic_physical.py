#!/usr/bin/env python3
"""Compose the fixed E016 intake through semantic and physical graphs."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_portable_c as intake


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONTRACT = FISICS_ROOT / "tests/fixtures/esp_portable_c/e016_composition_v1.json"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-portable-c"
FIXTURE_ROOT = FISICS_ROOT / "tests/fixtures/esp_portable_c"
TARGET_IDENTITY = "signal.bench_potentiometer.test_level"
PROJECT_ID = "esp.e016.sensor_metadata_bridge"
QUANTITY_ID = "quantity.normalized_signal.level"
PHYSICAL_CONTRACT_ID = "contract.esp.e016.normalized_signal.test_level"


def load_composition_contract(path: Path) -> dict[str, Any]:
    return load_composition_contract_from_data(intake.load_json(path))


def load_composition_contract_from_data(raw: Any) -> dict[str, Any]:
    data = intake.exact_keys(
        raw,
        {
            "schema", "schema_version", "lane_id", "intake_contract",
            "authored_inputs", "representation", "expected_graphs",
        },
        "composition contract",
    )
    intake.require(
        data["schema"] == "fisiCs.esp_semantic_physical_composition",
        "unsupported composition schema",
    )
    intake.require(data["schema_version"] == 1, "unsupported composition schema version")
    intake.require(data["lane_id"] == "esp.e016.sensor_metadata_bridge.v1", "composition lane drift")

    intake_contract = intake.exact_keys(
        data["intake_contract"], {"path", "sha256"}, "intake_contract"
    )
    intake.require(
        intake.relative_path(intake_contract["path"], "intake_contract.path")
        == "tests/fixtures/esp_portable_c/e016_intake_v1.json",
        "composition must use the fixed E016 intake contract",
    )
    intake.digest_string(intake_contract["sha256"], "intake_contract.sha256")

    authored = data["authored_inputs"]
    intake.require(isinstance(authored, list) and len(authored) == 3, "authored_inputs must contain exactly three files")
    expected_roles = {
        "semantic_project_manifest",
        "physical_contract_catalog",
        "physical_project_manifest",
    }
    roles: set[str] = set()
    paths: set[str] = set()
    for index, item in enumerate(authored):
        item = intake.exact_keys(item, {"role", "path", "sha256"}, f"authored_inputs[{index}]")
        role = intake.nonempty_string(item["role"], f"authored_inputs[{index}].role")
        path_value = intake.relative_path(item["path"], f"authored_inputs[{index}].path")
        intake.digest_string(item["sha256"], f"authored_inputs[{index}].sha256")
        intake.require(role not in roles, f"duplicate authored input role: {role}")
        intake.require(path_value not in paths, f"duplicate authored input path: {path_value}")
        roles.add(role)
        paths.add(path_value)
    intake.require(roles == expected_roles, f"authored input roles drift: {sorted(roles)}")

    representation = intake.exact_keys(
        data["representation"],
        {
            "subject_identity", "integer_minimum", "integer_maximum",
            "normalization_basis", "dimension", "calibration", "unit_policy",
            "value_form", "reference_semantics", "forbidden_interpretations",
        },
        "representation",
    )
    frozen_representation = {
        "subject_identity": TARGET_IDENTITY,
        "integer_minimum": 0,
        "integer_maximum": 1000,
        "normalization_basis": "filtered_adc_domain",
        "dimension": "dimensionless",
        "calibration": "uncalibrated",
        "unit_policy": "dimension_only",
        "value_form": "scalar",
        "reference_semantics": "linear",
        "forbidden_interpretations": [
            "voltage", "shaft_angle", "travel_distance", "calibrated_percentage"
        ],
    }
    intake.require(representation == frozen_representation, "dishonest or drifted normalized-signal representation")

    expected = intake.exact_keys(
        data["expected_graphs"],
        {
            "semantic_record_count", "semantic_edge_count", "semantic_edge_resolution",
            "physical_contract_count", "physical_contract_state", "physical_acceptance",
        },
        "expected_graphs",
    )
    frozen_expected = {
        "semantic_record_count": 2,
        "semantic_edge_count": 2,
        "semantic_edge_resolution": "external",
        "physical_contract_count": 1,
        "physical_contract_state": "external_declared",
        "physical_acceptance": "not_assessed",
    }
    intake.require(expected == frozen_expected, "graph expectation drift")
    return data


def authored_map(contract: dict[str, Any]) -> dict[str, dict[str, str]]:
    return {item["role"]: item for item in contract["authored_inputs"]}


def verify_authored_inputs(contract: dict[str, Any]) -> dict[str, Path]:
    paths: dict[str, Path] = {}
    for role, item in authored_map(contract).items():
        path = intake.canonical_child(FISICS_ROOT, item["path"], f"authored input {role}")
        intake.require(
            intake.sha256_path(path) == item["sha256"],
            f"authored input digest mismatch for {role}",
        )
        paths[role] = path

    semantic = intake.exact_keys(
        intake.load_json(paths["semantic_project_manifest"]),
        {"schema", "schema_version", "project_id", "translation_units", "allowed_external_targets"},
        "E016 semantic project manifest",
    )
    intake.require(semantic["schema"] == "fisiCs.semantic_project_manifest" and semantic["schema_version"] == 1, "semantic manifest identity drift")
    intake.require(semantic["project_id"] == PROJECT_ID, "semantic project identity drift")
    intake.require(
        semantic["translation_units"]
        == [{"metadata_path": "../../../build/integration-esp-portable-c/normalized_signal_policy.semantic.json"}],
        "semantic manifest must select exactly the fresh E016 translation-unit artifact",
    )
    intake.require(
        semantic["allowed_external_targets"] == [TARGET_IDENTITY],
        "semantic manifest must explicitly allow only the protocol-owned target",
    )

    catalog = intake.exact_keys(
        intake.load_json(paths["physical_contract_catalog"]),
        {"schema", "schema_version", "catalog_id", "quantity_kinds", "contracts"},
        "E016 physical contract catalog",
    )
    intake.require(catalog["schema"] == "fisiCs.physical_contract_catalog" and catalog["schema_version"] == 1, "physical catalog identity drift")
    intake.require(len(catalog["quantity_kinds"]) == 1 and len(catalog["contracts"]) == 1, "E016 physical catalog must contain exactly one quantity and contract")
    quantity = catalog["quantity_kinds"][0]
    intake.require(
        quantity == {
            "id": QUANTITY_ID,
            "dimension_system": "fisiCs.si_dim8.v1",
            "dimension_vector": [0, 0, 0, 0, 0, 0, 0, 0],
            "dimension_text": "dimensionless",
        },
        "dishonest or drifted dimensionless quantity declaration",
    )
    physical_contract = catalog["contracts"][0]
    intake.require(
        physical_contract == {
            "contract_id": PHYSICAL_CONTRACT_ID,
            "subject": {"kind": "semantic_id", "id": TARGET_IDENTITY},
            "quantity_kind": QUANTITY_ID,
            "unit_policy": {"mode": "dimension_only", "unit": None},
            "value_form": {"kind": "scalar", "component_count": None, "tensor_rank": None, "tensor_shape": []},
            "reference_semantics": {"kind": "linear", "reference_subject": None, "period_binary64": None},
            "frame_id": None,
        },
        "dishonest or drifted E016 physical contract",
    )

    physical_manifest = intake.exact_keys(
        intake.load_json(paths["physical_project_manifest"]),
        {
            "schema", "schema_version", "project_id", "semantic_project_graph_path",
            "units_metadata_paths", "physical_catalog_paths", "coverage_policy",
            "source_evidence_policy",
        },
        "E016 physical project manifest",
    )
    intake.require(physical_manifest == {
        "schema": "fisiCs.physical_project_manifest",
        "schema_version": 1,
        "project_id": PROJECT_ID,
        "semantic_project_graph_path": "../../../build/integration-esp-portable-c/e016.semantic-project.graph.json",
        "units_metadata_paths": ["../../../build/integration-esp-portable-c/normalized_signal_policy.units.json"],
        "physical_catalog_paths": ["e016.physical.catalog.json"],
        "coverage_policy": "require_relationship_targets",
        "source_evidence_policy": "report_unverified",
    }, "physical project manifest drift")
    return paths


def run(command: list[str], *, label: str, cwd: Path = FISICS_ROOT) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["FISICS_MAX_PROCS"] = "0"
    completed = subprocess.run(command, cwd=cwd, env=environment, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        intake.fail(f"{label} failed with exit {completed.returncode}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}")
    return completed


def expect_failure(command: list[str], *, needle: str, label: str) -> None:
    environment = os.environ.copy()
    environment["FISICS_MAX_PROCS"] = "0"
    completed = subprocess.run(command, cwd=FISICS_ROOT, env=environment, text=True, capture_output=True, check=False)
    intake.require(completed.returncode != 0, f"{label} unexpectedly succeeded")
    intake.require(needle in completed.stderr, f"{label} did not report {needle!r}: {completed.stderr}")


def verify_semantic_graph(path: Path, contract: dict[str, Any]) -> dict[str, Any]:
    graph = intake.load_json(path)
    intake.require(graph.get("schema") == "fisiCs.semantic_project_graph" and graph.get("complete") is True, "semantic project graph identity/completeness drift")
    intake.require(graph.get("project", {}).get("id") == PROJECT_ID, "semantic project graph project ID drift")
    expected = contract["expected_graphs"]
    intake.require(len(graph.get("records", [])) == expected["semantic_record_count"], "semantic project graph record count drift")
    intake.require(len(graph.get("edges", [])) == expected["semantic_edge_count"], "semantic project graph edge count drift")
    record_ids = {record.get("authored_id") for record in graph["records"]}
    intake.require(record_ids == {
        "operation.esp.normalized_signal.adc_normalize",
        "operation.esp.normalized_signal.publication_decide",
    }, "semantic project graph operation identities drift")
    for edge in graph["edges"]:
        intake.require(edge.get("target_id") == TARGET_IDENTITY, "semantic graph target identity drift")
        intake.require(edge.get("resolution") == "external", "semantic relationship was improperly promoted to local resolution")
        intake.require(edge.get("target_record_index") is None, "external relationship gained a local target record")
    return {
        "sha256": intake.sha256_path(path),
        "record_count": len(graph["records"]),
        "edge_count": len(graph["edges"]),
        "external_edge_count": len(graph["edges"]),
    }


def verify_units(path: Path, source: Path) -> dict[str, Any]:
    document = intake.load_json(path)
    intake.require(document.get("schema") == "fisiCs.units_metadata" and document.get("complete") is True, "units metadata identity/completeness drift")
    receipt = document.get("source", {})
    intake.require(Path(receipt.get("path", "")).resolve() == source.resolve(), "units metadata source path drift")
    intake.require(receipt.get("length_bytes") == source.stat().st_size, "units metadata source length drift")
    return {"sha256": intake.sha256_path(path), "attachment_count": len(document.get("attachments", []))}


def verify_physical_graph(path: Path, contract: dict[str, Any]) -> dict[str, Any]:
    graph = intake.load_json(path)
    expected = contract["expected_graphs"]
    intake.require(graph.get("schema") == "fisiCs.physical_project_graph" and graph.get("complete") is True, "physical project graph identity/completeness drift")
    intake.require(graph.get("physical_acceptance") == expected["physical_acceptance"], "physical graph improperly changed acceptance state")
    contracts = graph.get("physical_contracts", [])
    intake.require(len(contracts) == expected["physical_contract_count"], "physical graph contract count drift")
    item = contracts[0]
    intake.require(item.get("contract_id") == PHYSICAL_CONTRACT_ID, "physical contract identity drift")
    intake.require(item.get("subject") == {"kind": "semantic_id", "id": TARGET_IDENTITY}, "physical contract subject drift")
    intake.require(item.get("quantity_kind") == QUANTITY_ID, "physical quantity identity drift")
    intake.require(item.get("unit_policy") == {"mode": "dimension_only", "unit": None}, "physical unit policy drift")
    intake.require(item.get("value_form", {}).get("kind") == "scalar", "physical value form drift")
    intake.require(item.get("reference_semantics", {}).get("kind") == "linear", "physical reference semantics drift")
    intake.require(item.get("state") == expected["physical_contract_state"], "external physical contract state drift")
    intake.require(item.get("evidence_origin") == "catalog_declaration", "external physical contract gained source evidence")
    semantic_edges = graph.get("semantic_edges", [])
    intake.require(len(semantic_edges) == 2, "physical graph semantic edge count drift")
    intake.require(all(edge.get("target_id") == TARGET_IDENTITY and edge.get("target_contract_index") == 0 for edge in semantic_edges), "physical graph relationship binding drift")
    coverage = graph.get("coverage", {})
    intake.require(coverage.get("uncovered_relationship_targets") == 0, "physical graph left a relationship target uncovered")
    intake.require(coverage.get("external_declared_contracts") == 1, "physical graph external declaration coverage drift")
    return {
        "sha256": intake.sha256_path(path),
        "physical_contract_count": len(contracts),
        "semantic_edge_count": len(semantic_edges),
        "physical_contract_state": item["state"],
        "physical_acceptance": graph["physical_acceptance"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esp-work-root", required=True, type=Path)
    parser.add_argument("--compiler", type=Path, default=FISICS_ROOT / "fisics")
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    args = parser.parse_args()

    contract_path = args.contract.resolve()
    compiler = args.compiler.resolve()
    build_root = args.build_root.resolve()
    esp_root = args.esp_work_root.resolve()
    intake.require(contract_path == DEFAULT_CONTRACT.resolve(), "the runner accepts only the fixed Slice C contract")
    intake.require(build_root == DEFAULT_BUILD_ROOT.resolve(), "the runner accepts only the fixed Slice C build root")
    intake.require(compiler.is_file() and os.access(compiler, os.X_OK), f"compiler is not executable: {compiler}")
    intake.require((esp_root / ".git").exists(), f"ESP_WORK_ROOT is not a Git checkout: {esp_root}")
    git_root = intake.git(esp_root, "rev-parse", "--show-toplevel", label="ESP Git root").strip()
    intake.require(Path(git_root).resolve() == esp_root, f"ESP_WORK_ROOT is not the exact Git top level: {esp_root}")
    before_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status before Slice C")
    before_fingerprint, before_entry_count = intake.worktree_fingerprint(esp_root)
    contract = load_composition_contract(contract_path)
    authored = verify_authored_inputs(contract)
    intake_contract_path = intake.canonical_child(FISICS_ROOT, contract["intake_contract"]["path"], "Slice B intake contract")
    intake.require(intake.sha256_path(intake_contract_path) == contract["intake_contract"]["sha256"], "Slice B intake contract digest drift")
    intake_spec = intake.load_contract(intake_contract_path)
    verified_inputs = intake.verify_external_inputs(esp_root, intake_spec)
    intake.verify_protocol(esp_root, intake_spec)
    compiler_header = intake.canonical_child(
        FISICS_ROOT, intake_spec["compiler_header"]["path"], "compiler extension header"
    )
    intake.require(
        intake.sha256_path(compiler_header) == intake_spec["compiler_header"]["sha256"],
        "compiler extension-header digest drift",
    )
    vendored_header = intake.canonical_child(
        esp_root,
        intake.input_map(intake_spec)["vendored_extension_header"]["path"],
        "vendored extension header",
    )
    intake.require(compiler_header.read_bytes() == vendored_header.read_bytes(), "compiler and vendored extension headers differ")

    source_item = intake.input_map(intake_spec)[intake_spec["behavior"]["source_role"]]
    source = intake.canonical_child(esp_root, source_item["path"], "E016 production source")
    include_path = (esp_root / intake_spec["behavior"]["include_path"]).resolve()
    semantic_path = build_root / "normalized_signal_policy.semantic.json"

    with tempfile.TemporaryDirectory(prefix="fisics-e016-composition-") as temporary_name:
        temporary = Path(temporary_name)
        combined_semantic = temporary / "combined.semantic.json"
        units_path = temporary / "normalized_signal_policy.units.json"
        object_path = temporary / "normalized_signal_policy.o"
        run([
            str(compiler), "-std=c99", "--overlay=physics-units,semantic-metadata",
            f"-I{include_path}", "--emit-semantic-metadata-json", str(combined_semantic),
            "--emit-units-metadata-json", str(units_path), "-c", str(source), "-o", str(object_path),
        ], label="combined semantic and units evidence regeneration")
        metadata_receipt = intake.verify_metadata(combined_semantic, source, intake_spec)
        units_receipt = verify_units(units_path, source)
        intake.atomic_write(semantic_path, combined_semantic.read_bytes())
        intake.atomic_write(build_root / "normalized_signal_policy.units.json", units_path.read_bytes())

        semantic_one = temporary / "semantic.one.json"
        semantic_two = temporary / "semantic.two.json"
        semantic_command = [str(compiler), "--semantic-project-manifest", str(authored["semantic_project_manifest"]), "--emit-semantic-project-graph-json"]
        run([*semantic_command, str(semantic_one)], label="semantic project graph run 1")
        run([*semantic_command, str(semantic_two)], label="semantic project graph run 2")
        intake.require(semantic_one.read_bytes() == semantic_two.read_bytes(), "semantic project graph is not byte deterministic")
        semantic_receipt = verify_semantic_graph(semantic_one, contract)
        intake.atomic_write(build_root / "e016.semantic-project.graph.json", semantic_one.read_bytes())

        unapproved_manifest = temporary / "unapproved.semantic-project.json"
        unapproved = intake.load_json(authored["semantic_project_manifest"])
        unapproved["translation_units"] = [{"metadata_path": str(semantic_path)}]
        unapproved["allowed_external_targets"] = []
        intake.atomic_write(unapproved_manifest, (json.dumps(unapproved, indent=2) + "\n").encode())
        expect_failure([
            str(compiler), "--semantic-project-manifest", str(unapproved_manifest),
            "--emit-semantic-project-graph-json", str(temporary / "unapproved.graph.json"),
        ], needle=f"missing required semantic target {TARGET_IDENTITY}", label="unapproved external target negative")

        stale_metadata = temporary / "stale.semantic.json"
        stale = intake.load_json(semantic_path)
        stale["source"]["hash_fnv1a64"] = "0000000000000000"
        intake.atomic_write(stale_metadata, (json.dumps(stale, indent=2) + "\n").encode())
        stale_manifest = temporary / "stale.semantic-project.json"
        stale_project = intake.load_json(authored["semantic_project_manifest"])
        stale_project["translation_units"] = [{"metadata_path": str(stale_metadata)}]
        intake.atomic_write(stale_manifest, (json.dumps(stale_project, indent=2) + "\n").encode())
        expect_failure([
            str(compiler), "--semantic-project-manifest", str(stale_manifest),
            "--emit-semantic-project-graph-json", str(temporary / "stale.semantic.graph.json"),
        ], needle="stale semantic metadata source hash", label="stale translation-unit provenance negative")

        physical_one = temporary / "physical.one.json"
        physical_two = temporary / "physical.two.json"
        physical_command = [str(compiler), "--physical-project-manifest", str(authored["physical_project_manifest"]), "--emit-physical-project-graph-json"]
        run([*physical_command, str(physical_one)], label="physical composition graph run 1")
        run([*physical_command, str(physical_two)], label="physical composition graph run 2")
        intake.require(physical_one.read_bytes() == physical_two.read_bytes(), "physical composition graph is not byte deterministic")
        physical_receipt = verify_physical_graph(physical_one, contract)
        intake.atomic_write(build_root / "e016.physical-project.graph.json", physical_one.read_bytes())

        stale_units = temporary / "stale.units.json"
        stale_units_document = intake.load_json(units_path)
        stale_units_document["source"]["hash_fnv1a64"] = "0000000000000000"
        intake.atomic_write(stale_units, (json.dumps(stale_units_document, indent=2) + "\n").encode())
        stale_physical_manifest = temporary / "stale.physical-project.json"
        stale_physical = intake.load_json(authored["physical_project_manifest"])
        stale_physical["semantic_project_graph_path"] = str(build_root / "e016.semantic-project.graph.json")
        stale_physical["units_metadata_paths"] = [str(stale_units)]
        stale_physical["physical_catalog_paths"] = [str(authored["physical_contract_catalog"])]
        intake.atomic_write(stale_physical_manifest, (json.dumps(stale_physical, indent=2) + "\n").encode())
        expect_failure([
            str(compiler), "--physical-project-manifest", str(stale_physical_manifest),
            "--emit-physical-project-graph-json", str(temporary / "stale.physical.graph.json"),
        ], needle="stale physical units source hash", label="stale units provenance negative")

    after_inputs = intake.verify_external_inputs(esp_root, intake_spec)
    intake.require(after_inputs == verified_inputs, "selected ESP input bytes changed during Slice C")
    after_fingerprint, after_entry_count = intake.worktree_fingerprint(esp_root)
    after_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status after Slice C")
    intake.require(after_fingerprint == before_fingerprint, "ESP worktree bytes changed during Slice C")
    intake.require(after_entry_count == before_entry_count, "ESP worktree entry count changed during Slice C")
    intake.require(after_status == before_status, "ESP Git status changed during Slice C")

    receipt = {
        "schema": "fisiCs.esp_semantic_physical_composition_receipt",
        "schema_version": 1,
        "lane_id": contract["lane_id"],
        "status": "pass",
        "contract": {"path": str(contract_path), "sha256": intake.sha256_path(contract_path)},
        "external_repository": {
            "path": str(esp_root),
            "head": intake.git(esp_root, "rev-parse", "HEAD", label="ESP HEAD").strip(),
            "status_sha256": intake.sha256_bytes(before_status.encode("utf-8")),
            "worktree_entry_count": before_entry_count,
            "worktree_fingerprint_sha256": before_fingerprint,
            "unchanged_during_slice_c": True,
        },
        "authored_inputs": [
            {"role": item["role"], "path": item["path"], "sha256": item["sha256"]}
            for item in contract["authored_inputs"]
        ],
        "translation_unit_metadata": metadata_receipt,
        "units_metadata": units_receipt,
        "semantic_project_graph": semantic_receipt,
        "physical_composition_graph": physical_receipt,
        "representation": contract["representation"],
        "negative_proof": {
            "stale_translation_unit_provenance": "rejected",
            "stale_units_provenance": "rejected",
            "unapproved_external_target": "rejected",
            "dishonest_physical_typing": "rejected_by_fixed_contract",
        },
        "claim_boundary": {
            "semantic_relationship_resolution": "established",
            "physical_contract_compatibility": "established",
            "hardware_compatibility": "not_assessed",
            "calibration": "not_assessed",
            "deployment": "not_assessed",
            "runtime_observation": "not_assessed",
            "physical_acceptance": "not_assessed",
        },
    }
    intake.atomic_write(
        build_root / "e016.composition.receipt.json",
        (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode(),
    )
    print("E016 Slice C semantic=PASS external_edges=2 physical=PASS state=external_declared deterministic=PASS negatives=PASS")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"E016 Slice C: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
