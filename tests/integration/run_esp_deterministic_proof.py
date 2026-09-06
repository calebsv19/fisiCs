#!/usr/bin/env python3
"""Run the fixed E016 Slice E deterministic positive and negative proof."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from copy import deepcopy
from pathlib import Path
from typing import Any, Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_hardware_compatibility as hardware
import run_esp_portable_c as intake
import run_esp_semantic_physical as composition


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONTRACT = FISICS_ROOT / "tests/fixtures/esp_portable_c/e016_proof_v1.json"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-portable-c"
LIVE_EVIDENCE_KEYS = (
    "hardware_presence", "wiring", "driver_implementation", "calibration",
    "runtime_observation", "simulation", "deployment", "physical_acceptance",
)


def load_proof_contract(path: Path) -> dict[str, Any]:
    data = intake.exact_keys(
        intake.load_json(path),
        {
            "schema", "schema_version", "lane_id", "prerequisite_contracts",
            "terminal_graphs", "expected_corpus", "required_negative_classes",
        },
        "E016 proof contract",
    )
    intake.require(data["schema"] == "fisiCs.esp_deterministic_proof", "unsupported E016 proof schema")
    intake.require(data["schema_version"] == 1, "unsupported E016 proof schema version")
    intake.require(data["lane_id"] == "esp.e016.deterministic_proof.v1", "E016 proof lane drift")

    def fixed_inputs(items: Any, expected: dict[str, str], label: str) -> None:
        intake.require(isinstance(items, list) and len(items) == len(expected), f"{label} count drift")
        actual: dict[str, str] = {}
        for index, item in enumerate(items):
            item = intake.exact_keys(item, {"role", "path", "sha256"}, f"{label}[{index}]")
            role = intake.nonempty_string(item["role"], f"{label}[{index}].role")
            path_value = intake.relative_path(item["path"], f"{label}[{index}].path")
            intake.digest_string(item["sha256"], f"{label}[{index}].sha256")
            intake.require(role not in actual, f"duplicate {label} role: {role}")
            actual[role] = path_value
        intake.require(actual == expected, f"{label} role/path drift")

    fixed_inputs(data["prerequisite_contracts"], {
        "portable_c_intake": "tests/fixtures/esp_portable_c/e016_intake_v1.json",
        "semantic_physical_composition": "tests/fixtures/esp_portable_c/e016_composition_v1.json",
        "hardware_compatibility": "tests/fixtures/esp_portable_c/e016_hardware_v1.json",
    }, "prerequisite_contracts")
    fixed_inputs(data["terminal_graphs"], {
        "semantic_project_graph": "build/integration-esp-portable-c/e016.semantic-project.graph.json",
        "physical_composition_graph": "build/integration-esp-portable-c/e016.physical-project.graph.json",
        "hardware_compatibility_graph": "build/integration-esp-portable-c/e016.hardware-project.graph.json",
    }, "terminal_graphs")

    expected_corpus = intake.exact_keys(
        data["expected_corpus"],
        {
            "semantic_record_ids", "semantic_edges", "physical_contract_id",
            "physical_quantity_id", "physical_state", "node_ids", "bus_ids",
            "device_ids", "connection_ids", "binding_ids", "live_evidence_state",
        },
        "expected_corpus",
    )
    intake.require(expected_corpus["live_evidence_state"] == "not_assessed", "E016 proof attempted live-evidence promotion")
    required = {
        "stale_source_digest", "stale_protocol_digest", "unapproved_relationship_target",
        "conflicting_duplicate_identity", "incompatible_physical_quantity",
        "incompatible_value_form", "incompatible_unit_policy", "incompatible_reference_kind",
        "wrong_board_revision", "wrong_device_revision", "wrong_driver",
        "wrong_bus_revision", "wrong_endpoint_capability", "exclusive_gpio_collision",
        "missing_required_allocation", "missing_required_physical_binding",
        "static_live_evidence_promotion",
    }
    classes = data["required_negative_classes"]
    intake.require(isinstance(classes, list) and len(classes) == len(set(classes)), "required negative classes must be a unique array")
    intake.require(set(classes) == required, "required negative class coverage drift")
    return data


def item_map(items: list[dict[str, str]]) -> dict[str, dict[str, str]]:
    return {item["role"]: item for item in items}


def run(command: list[str], *, label: str) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["FISICS_MAX_PROCS"] = "0"
    completed = subprocess.run(command, cwd=FISICS_ROOT, env=environment, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        intake.fail(f"{label} failed with exit {completed.returncode}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}")
    return completed


def expect_runtime_failure(action: Callable[[], None], *, needle: str, label: str) -> None:
    try:
        action()
    except RuntimeError as exc:
        intake.require(needle in str(exc), f"{label} emitted an unexpected diagnostic: {exc}")
        return
    intake.fail(f"{label} unexpectedly succeeded")


def expect_compiler_failure(compiler: Path, manifest: Path, mode: str, output: Path, needles: tuple[str, ...], label: str) -> None:
    hardware.expect_failure(
        [str(compiler), mode, str(manifest), {
            "--semantic-project-manifest": "--emit-semantic-project-graph-json",
            "--physical-project-manifest": "--emit-physical-project-graph-json",
            "--hardware-project-manifest": "--emit-hardware-project-graph-json",
        }[mode], str(output)],
        needles=needles,
        label=label,
    )


def verify_positive_corpus(graphs: dict[str, Path], contract: dict[str, Any]) -> dict[str, str]:
    expected = contract["expected_corpus"]
    semantic = intake.load_json(graphs["semantic_project_graph"])
    record_ids = [item["authored_id"] for item in semantic["records"]]
    intake.require(record_ids == expected["semantic_record_ids"], "Slice E semantic record corpus drift")
    identities = {item["record_index"]: item["authored_id"] for item in semantic["records"]}
    edges = [[identities[item["source_record_index"]], item["kind"], item["target_id"], item["resolution"]] for item in semantic["edges"]]
    intake.require(edges == expected["semantic_edges"], "Slice E semantic edge corpus drift")

    physical = intake.load_json(graphs["physical_composition_graph"])
    intake.require([item["id"] for item in physical["quantity_kinds"]] == [expected["physical_quantity_id"]], "Slice E physical quantity corpus drift")
    intake.require(len(physical["physical_contracts"]) == 1, "Slice E physical contract count drift")
    physical_contract = physical["physical_contracts"][0]
    intake.require(physical_contract["contract_id"] == expected["physical_contract_id"], "Slice E physical contract identity drift")
    intake.require(physical_contract["state"] == expected["physical_state"], "Slice E physical evidence state drift")
    intake.require(physical_contract["unit_policy"] == {"mode": "dimension_only", "unit": None}, "Slice E unit policy drift")
    intake.require(physical_contract["value_form"]["kind"] == "scalar", "Slice E value form drift")
    intake.require(physical_contract["reference_semantics"]["kind"] == "linear", "Slice E reference kind drift")
    intake.require(physical["physical_acceptance"] == expected["live_evidence_state"], "Slice E physical graph promoted acceptance")

    graph = intake.load_json(graphs["hardware_compatibility_graph"])
    intake.require([item["node_id"] for item in graph["nodes"]] == expected["node_ids"], "Slice E node corpus drift")
    intake.require([item["bus_id"] for item in graph["buses"]] == expected["bus_ids"], "Slice E bus corpus drift")
    intake.require([item["device_id"] for item in graph["devices"]] == expected["device_ids"], "Slice E device corpus drift")
    intake.require([item["connection_id"] for item in graph["connections"]] == expected["connection_ids"], "Slice E connection corpus drift")
    intake.require([item["binding_id"] for item in graph["signal_bindings"]] == expected["binding_ids"], "Slice E binding corpus drift")
    intake.require(graph["coverage"] == {"unbound_required_ports": 0}, "Slice E required-port coverage drift")
    for binding in graph["signal_bindings"]:
        intake.require(binding["physical_contract_id"] == expected["physical_contract_id"], "Slice E hardware binding contract drift")
        intake.require(binding["physical_evidence_state"] == expected["physical_state"], "Slice E binding evidence promotion")
        intake.require(binding["compatibility_mode"] == "exact_quantity" and binding["result"] == "compatible", "Slice E binding compatibility drift")
    for key in LIVE_EVIDENCE_KEYS:
        intake.require(graph["assessment"][key] == expected["live_evidence_state"], f"Slice E hardware graph promoted {key}")
    return {role: intake.sha256_path(path) for role, path in graphs.items()}


def write_semantic_duplicate(temporary: Path, semantic_path: Path, manifest_path: Path) -> Path:
    conflict = intake.load_json(semantic_path)
    source = Path(conflict["source"]["path"])
    conflict_source = (temporary / "conflicting-normalized-signal-policy.c").resolve()
    intake.atomic_write(conflict_source, source.read_bytes())
    conflict["source"]["path"] = str(conflict_source)
    for record in conflict["records"]:
        record["source"]["file"] = str(conflict_source)
    for edge in conflict["edges"]:
        edge["occurrence"]["file"] = str(conflict_source)
    conflict_path = temporary / "conflicting.semantic.json"
    intake.atomic_write(conflict_path, (json.dumps(conflict, indent=2) + "\n").encode())
    manifest = intake.load_json(manifest_path)
    manifest["translation_units"] = [{"metadata_path": str(semantic_path)}, {"metadata_path": str(conflict_path)}]
    output = temporary / "conflicting.semantic-project.json"
    intake.atomic_write(output, (json.dumps(manifest, indent=2) + "\n").encode())
    return output


def write_hardware_case(temporary: Path, authored: dict[str, Path], physical_graph: Path, name: str, mutate: Callable[[dict[str, Any]], None], source_role: str = "hardware_topology") -> Path:
    return hardware.write_mutated_case(temporary, authored, physical_graph, name, mutate, source_role)


def write_gpio_collision_case(temporary: Path, authored: dict[str, Path], physical_graph: Path) -> Path:
    topology = intake.load_json(authored["hardware_topology"])
    catalog = intake.load_json(authored["hardware_component_catalog"])
    endpoint = next(item for item in catalog["board_profiles"][0]["endpoints"] if item["endpoint_id"] == "gpio1")
    endpoint["capability_ids"].append("endpoint.twai.tx")
    connection = next(item for item in topology["connections"] if item["connection_id"] == "connection.bench_b.tja1050.txd")
    connection["target"]["board_endpoint_id"] = "gpio1"
    topology_path = temporary / "gpio-collision.topology.json"
    catalog_path = temporary / "gpio-collision.catalog.json"
    intake.atomic_write(topology_path, (json.dumps(topology, indent=2) + "\n").encode())
    intake.atomic_write(catalog_path, (json.dumps(catalog, indent=2) + "\n").encode())
    manifest = intake.load_json(authored["hardware_project_manifest"])
    manifest["physical_project_graph_path"] = str(physical_graph)
    manifest["hardware_catalog_paths"] = [str(catalog_path)]
    manifest["hardware_topology_path"] = str(topology_path)
    manifest_path = temporary / "gpio-collision.manifest.json"
    intake.atomic_write(manifest_path, (json.dumps(manifest, indent=2) + "\n").encode())
    return manifest_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esp-work-root", required=True, type=Path)
    parser.add_argument("--compiler", type=Path, default=FISICS_ROOT / "fisics")
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    args = parser.parse_args()

    esp_root = args.esp_work_root.resolve()
    compiler = args.compiler.resolve()
    contract_path = args.contract.resolve()
    build_root = args.build_root.resolve()
    intake.require(contract_path == DEFAULT_CONTRACT.resolve(), "the runner accepts only the fixed Slice E contract")
    intake.require(build_root == DEFAULT_BUILD_ROOT.resolve(), "the runner accepts only the fixed Slice E build root")
    intake.require(compiler.is_file() and os.access(compiler, os.X_OK), f"compiler is not executable: {compiler}")
    intake.require((esp_root / ".git").exists(), f"ESP_WORK_ROOT is not a Git checkout: {esp_root}")
    intake.require(Path(intake.git(esp_root, "rev-parse", "--show-toplevel", label="ESP Git root").strip()).resolve() == esp_root, "ESP_WORK_ROOT is not the exact Git top level")
    before_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status before Slice E")
    before_fingerprint, before_entry_count = intake.worktree_fingerprint(esp_root)

    contract = load_proof_contract(contract_path)
    prerequisites = item_map(contract["prerequisite_contracts"])
    for item in prerequisites.values():
        path = intake.canonical_child(FISICS_ROOT, item["path"], f"Slice E prerequisite {item['role']}")
        intake.require(intake.sha256_path(path) == item["sha256"], f"Slice E prerequisite digest mismatch for {item['role']}")

    hardware_contract = intake.canonical_child(FISICS_ROOT, prerequisites["hardware_compatibility"]["path"], "Slice D contract")
    run([
        sys.executable, str(FISICS_ROOT / "tests/integration/run_esp_hardware_compatibility.py"),
        "--esp-work-root", str(esp_root), "--compiler", str(compiler),
        "--contract", str(hardware_contract), "--build-root", str(build_root),
    ], label="Slice D prerequisite regeneration")

    terminal_items = item_map(contract["terminal_graphs"])
    graphs: dict[str, Path] = {}
    for role, item in terminal_items.items():
        path = intake.canonical_child(FISICS_ROOT, item["path"], f"Slice E terminal graph {role}")
        intake.require(intake.sha256_path(path) == item["sha256"], f"Slice E terminal graph digest drift for {role}")
        graphs[role] = path
    graph_digests = verify_positive_corpus(graphs, contract)

    intake_contract_path = intake.canonical_child(FISICS_ROOT, prerequisites["portable_c_intake"]["path"], "Slice A contract")
    intake_contract = intake.load_contract(intake_contract_path)
    composition_contract = intake.load_json(intake.canonical_child(FISICS_ROOT, prerequisites["semantic_physical_composition"]["path"], "Slice C contract"))
    hardware_spec = hardware.load_hardware_contract(hardware_contract)
    authored = hardware.verify_pinned_files(FISICS_ROOT, hardware_spec["authored_inputs"], "Slice E authored hardware input")

    with tempfile.TemporaryDirectory(prefix="fisics-e016-proof-") as temporary_name:
        temporary = Path(temporary_name)

        stale_source = deepcopy(intake_contract)
        next(item for item in stale_source["external_inputs"] if item["role"] == "production_source")["sha256"] = "0" * 64
        expect_runtime_failure(lambda: intake.verify_external_inputs(esp_root, stale_source), needle="external input digest mismatch for production_source", label="stale source digest negative")

        stale_protocol = deepcopy(intake_contract)
        stale_protocol["protocol"]["canonical_protocol_sha256"] = "0" * 64
        expect_runtime_failure(lambda: intake.verify_protocol(esp_root, stale_protocol), needle="canonical protocol digest drift", label="stale protocol digest negative")

        semantic_manifest = intake.canonical_child(FISICS_ROOT, "tests/fixtures/esp_portable_c/e016.semantic-project.json", "E016 semantic manifest")
        duplicate_manifest = write_semantic_duplicate(temporary, build_root / "normalized_signal_policy.semantic.json", semantic_manifest)
        expect_compiler_failure(compiler, duplicate_manifest, "--semantic-project-manifest", temporary / "duplicate.graph.json", ("duplicate project authored_id",), "conflicting duplicate identity negative")

        physical_graph = graphs["physical_composition_graph"]
        hardware_cases = [
            ("quantity", lambda data: next(item for item in data["device_models"] if item["model_id"] == "device.codework_esp.e016.observer")["physical_ports"][0]["requirement"].update({"quantity_kind": "quantity.incompatible"}), ("quantity mismatch",)),
            ("value-form", lambda data: next(item for item in data["device_models"] if item["model_id"] == "device.codework_esp.e016.observer")["physical_ports"][0]["requirement"].update({"value_form": "vector"}), ("value form or reference mismatch",)),
            ("reference-kind", lambda data: next(item for item in data["device_models"] if item["model_id"] == "device.codework_esp.e016.observer")["physical_ports"][0]["requirement"].update({"reference_kind": "relative"}), ("value form or reference mismatch",)),
        ]
        for name, mutate, needles in hardware_cases:
            manifest = write_hardware_case(temporary, authored, physical_graph, name, mutate, "hardware_component_catalog")
            expect_compiler_failure(compiler, manifest, "--hardware-project-manifest", temporary / f"{name}.graph.json", needles, f"{name} negative")
        gpio_collision_manifest = write_gpio_collision_case(temporary, authored, physical_graph)
        expect_compiler_failure(compiler, gpio_collision_manifest, "--hardware-project-manifest", temporary / "gpio-collision.graph.json", ("exclusive direct endpoint conflict",), "gpio-collision negative")

        unit_policy = deepcopy(composition_contract)
        unit_policy["representation"]["unit_policy"] = "exact_unit"
        expect_runtime_failure(lambda: composition.load_composition_contract_from_data(unit_policy), needle="dishonest or drifted", label="unit policy negative")

        promoted = deepcopy(contract)
        promoted["expected_corpus"]["live_evidence_state"] = "passed"
        temporary_contract = temporary / "promoted-proof.json"
        intake.atomic_write(temporary_contract, (json.dumps(promoted, indent=2) + "\n").encode())
        expect_runtime_failure(lambda: load_proof_contract(temporary_contract), needle="live-evidence promotion", label="static live-evidence promotion negative")

    composition_receipt = intake.load_json(build_root / "e016.composition.receipt.json")
    hardware_receipt = intake.load_json(build_root / "e016.hardware-compatibility.receipt.json")
    inherited_negative = {**composition_receipt["negative_proof"], **hardware_receipt["negative_proof"]}
    inherited_requirements = {
        "stale_translation_unit_provenance", "stale_units_provenance",
        "unapproved_external_target", "dishonest_physical_typing",
        "wrong_board_revision", "wrong_device_revision", "wrong_driver",
        "wrong_bus_revision", "wrong_endpoint_capability", "missing_allocation",
        "missing_required_binding", "missing_physical_contract",
    }
    intake.require(inherited_requirements <= set(inherited_negative), "Slice E prerequisite negative-proof coverage drift")
    intake.require(all(inherited_negative[key].startswith("rejected") for key in inherited_requirements), "Slice E prerequisite negative proof did not reject every case")
    negative_proof = {name: "rejected" for name in contract["required_negative_classes"]}

    after_fingerprint, after_entry_count = intake.worktree_fingerprint(esp_root)
    after_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status after Slice E")
    intake.require(after_fingerprint == before_fingerprint, "ESP worktree bytes changed during Slice E")
    intake.require(after_entry_count == before_entry_count, "ESP worktree entry count changed during Slice E")
    intake.require(after_status == before_status, "ESP Git status changed during Slice E")

    receipt = {
        "schema": "fisiCs.esp_deterministic_proof_receipt",
        "schema_version": 1,
        "lane_id": contract["lane_id"],
        "status": "pass",
        "contract": {"path": str(contract_path), "sha256": intake.sha256_path(contract_path)},
        "external_repository": {
            "path": str(esp_root),
            "head": intake.git(esp_root, "rev-parse", "HEAD", label="ESP HEAD").strip(),
            "status_sha256": intake.sha256_bytes(before_status.encode()),
            "worktree_entry_count": before_entry_count,
            "worktree_fingerprint_sha256": before_fingerprint,
            "unchanged_during_slice_e": True,
        },
        "terminal_graph_sha256": graph_digests,
        "determinism": {role: "byte_identical_across_two_emissions" for role in graph_digests},
        "positive_corpus": "exact",
        "negative_proof": negative_proof,
        "inherited_prerequisite_negative_proof": inherited_negative,
        "claim_boundary": {
            "portable_c_host_parity": "established",
            "semantic_relationship_resolution": "established",
            "physical_contract_compatibility": "established",
            "static_hardware_compatibility": "established",
            **{key: "not_assessed" for key in LIVE_EVIDENCE_KEYS},
        },
    }
    receipt_bytes = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode()
    intake.atomic_write(build_root / "e016.deterministic-proof.receipt.json", receipt_bytes)
    print(f"E016 Slice E positive=PASS deterministic=PASS negatives={len(negative_proof)} live_claims=not_assessed external_tree=UNCHANGED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"E016 Slice E: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
