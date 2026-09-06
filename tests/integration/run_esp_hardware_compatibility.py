#!/usr/bin/env python3
"""Build the fixed E016 static hardware compatibility graph."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any, Callable

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_portable_c as intake
import run_esp_semantic_physical as composition


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONTRACT = FISICS_ROOT / "tests/fixtures/esp_portable_c/e016_hardware_v1.json"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-portable-c"
PROJECT_ID = "esp.e016.sensor_metadata_bridge"
PHYSICAL_CONTRACT_ID = "contract.esp.e016.normalized_signal.test_level"


def load_hardware_contract(path: Path) -> dict[str, Any]:
    return load_hardware_contract_from_data(intake.load_json(path))


def load_hardware_contract_from_data(raw: Any) -> dict[str, Any]:
    data = intake.exact_keys(
        raw,
        {
            "schema", "schema_version", "lane_id", "slice_c", "external_inputs",
            "authored_inputs", "declared_scope", "expected_graph",
        },
        "E016 hardware contract",
    )
    intake.require(data["schema"] == "fisiCs.esp_hardware_compatibility", "unsupported E016 hardware contract schema")
    intake.require(data["schema_version"] == 1, "unsupported E016 hardware contract version")
    intake.require(data["lane_id"] == "esp.e016.hardware_compatibility.v1", "E016 hardware lane drift")

    slice_c = intake.exact_keys(
        data["slice_c"],
        {"contract_path", "contract_sha256", "physical_graph_path", "physical_graph_sha256"},
        "slice_c",
    )
    intake.require(intake.relative_path(slice_c["contract_path"], "slice_c.contract_path") == "tests/fixtures/esp_portable_c/e016_composition_v1.json", "Slice D must consume the fixed Slice C contract")
    intake.require(intake.relative_path(slice_c["physical_graph_path"], "slice_c.physical_graph_path") == "build/integration-esp-portable-c/e016.physical-project.graph.json", "Slice D physical graph path drift")
    intake.digest_string(slice_c["contract_sha256"], "slice_c.contract_sha256")
    intake.digest_string(slice_c["physical_graph_sha256"], "slice_c.physical_graph_sha256")

    def validate_inputs(items: Any, expected_roles: set[str], label: str) -> None:
        intake.require(isinstance(items, list) and len(items) == len(expected_roles), f"{label} must contain exactly {len(expected_roles)} inputs")
        roles: set[str] = set()
        paths: set[str] = set()
        for index, item in enumerate(items):
            item = intake.exact_keys(item, {"role", "path", "sha256"}, f"{label}[{index}]")
            role = intake.nonempty_string(item["role"], f"{label}[{index}].role")
            path_value = intake.relative_path(item["path"], f"{label}[{index}].path")
            intake.digest_string(item["sha256"], f"{label}[{index}].sha256")
            intake.require(role not in roles and path_value not in paths, f"duplicate {label} role or path")
            roles.add(role)
            paths.add(path_value)
        intake.require(roles == expected_roles, f"{label} roles drift: {sorted(roles)}")

    validate_inputs(
        data["external_inputs"],
        {
            "bench_a_profile", "bench_b_profile", "experiment_contract",
            "can_bench_contract", "board_contract", "protocol_schema",
        },
        "external_inputs",
    )
    validate_inputs(
        data["authored_inputs"],
        {"hardware_component_catalog", "hardware_topology", "hardware_project_manifest"},
        "authored_inputs",
    )

    frozen_scope = {
        "board_profile": "board.espressif.esp32_c6_devkitc_1",
        "board_revision": "wroom-1",
        "nodes": ["node.bench_a", "node.bench_b"],
        "bus_profile": "bus.can.classical.kbps500",
        "bus_scope": "project_shared",
        "transceiver_model": "device.nxp.tja1050",
        "transceiver_revision": "declared-breakout-v1",
        "bench_b_potentiometer_endpoint": "gpio1",
        "twai_tx_endpoint": "gpio10",
        "twai_rx_endpoint": "gpio11",
        "twai_rx_protection": "declared_external_divider",
        "producer_binding": "binding.bench_b.e016_producer",
        "consumer_binding": "binding.bench_a.e016_consumer",
        "physical_contract": PHYSICAL_CONTRACT_ID,
        "can_network_anchor": "structural_only_not_bus_master",
        "electrical_declarations_outside_graph_v1": [
            "canh_continuity", "canl_continuity", "common_can_ground",
            "separate_local_5v_domains", "potentiometer_local_3v3_supply",
            "termination_resistance", "rx_divider_values",
        ],
    }
    intake.require(data["declared_scope"] == frozen_scope, "E016 declared hardware scope drift")

    frozen_expected = {
        "bus_profile_count": 1,
        "board_profile_count": 1,
        "device_model_count": 4,
        "driver_contract_count": 2,
        "node_count": 2,
        "bus_count": 1,
        "device_count": 5,
        "connection_count": 9,
        "signal_binding_count": 2,
        "unbound_required_ports": 0,
        "physical_evidence_state": "external_declared",
        "declared_topology": "compatible",
        "hardware_presence": "not_assessed",
        "wiring": "not_assessed",
        "driver_implementation": "not_assessed",
        "calibration": "not_assessed",
        "runtime_observation": "not_assessed",
        "deployment": "not_assessed",
        "physical_acceptance": "not_assessed",
    }
    intake.require(data["expected_graph"] == frozen_expected, "E016 hardware graph expectation drift")
    return data


def role_map(items: list[dict[str, str]]) -> dict[str, dict[str, str]]:
    return {item["role"]: item for item in items}


def verify_pinned_files(root: Path, items: list[dict[str, str]], label: str) -> dict[str, Path]:
    verified: dict[str, Path] = {}
    for item in items:
        path = intake.canonical_child(root, item["path"], f"{label} {item['role']}")
        intake.require(intake.sha256_path(path) == item["sha256"], f"{label} digest mismatch for {item['role']}")
        verified[item["role"]] = path
    return verified


def verify_external_authority(paths: dict[str, Path]) -> None:
    bench_a = intake.load_json(paths["bench_a_profile"])
    bench_b = intake.load_json(paths["bench_b_profile"])
    intake.require(bench_a.get("node") == "BENCH_A" and bench_a.get("role") == "bench_a_pot_observer", "Bench A E016 profile drift")
    intake.require(bench_b.get("node") == "BENCH_B" and bench_b.get("role") == "bench_b_pot_publisher", "Bench B E016 profile drift")
    intake.require(bench_a.get("safety", {}).get("can_transport") == "twai_500k_candidate", "Bench A CAN profile drift")
    intake.require(bench_b.get("safety", {}).get("can_transport") == "twai_500k_candidate", "Bench B CAN profile drift")
    protocol = intake.load_json(paths["protocol_schema"])
    intake.require(protocol.get("wire", {}).get("can") == "classical", "protocol no longer declares Classical CAN")
    signal = next((item for item in protocol.get("semantic_types", []) if item.get("name") == "TEST_LEVEL"), None)
    intake.require(signal is not None and signal.get("raw_minimum") == 0 and signal.get("raw_maximum") == 1000 and signal.get("unit") == "normalized", "TEST_LEVEL protocol representation drift")
    experiment = paths["experiment_contract"].read_text(encoding="utf-8")
    for statement in (
        "Sensor ADC: Bench B GPIO1",
        "GPIO10 to local TJA `TX`",
        "Protected local TJA `RX` divider junction to GPIO11",
    ):
        intake.require(statement in experiment, f"E016 experiment no longer contains required declaration: {statement}")
    can_bench = paths["can_bench_contract"].read_text(encoding="utf-8")
    for statement in (
        "Boards: two ESP32-C6-DevKitC-1 boards",
        "Modules: two WWZMDiB TJA1050 breakout modules",
        "Bitrate: 500 kbit/s",
        "TWAI pins: TX GPIO10, RX GPIO11",
    ):
        intake.require(statement in can_bench, f"E013 CAN bench no longer contains required declaration: {statement}")
    board = paths["board_contract"].read_text(encoding="utf-8")
    intake.require("Module marking: ESP32-C6-WROOM-1" in board, "ESP32-C6 module declaration drift")


def verify_authored_inputs(paths: dict[str, Path]) -> None:
    catalog = intake.load_json(paths["hardware_component_catalog"])
    topology = intake.load_json(paths["hardware_topology"])
    manifest = intake.load_json(paths["hardware_project_manifest"])
    intake.require(catalog.get("schema") == "fisiCs.hardware_component_catalog" and catalog.get("schema_version") == 1, "E016 hardware catalog identity drift")
    intake.require(catalog.get("catalog_id") == "catalog.esp.e016.declared_components", "E016 hardware catalog ID drift")
    intake.require(catalog.get("authority") == {"kind": "project", "identity": "authority.esp.e016.bridge"}, "E016 catalog authority drift")
    intake.require([item.get("profile_id") for item in catalog.get("bus_profiles", [])] == ["bus.can.classical.kbps500"], "E016 bus profile drift")
    intake.require([item.get("profile_id") for item in catalog.get("board_profiles", [])] == ["board.espressif.esp32_c6_devkitc_1"], "E016 board profile drift")
    intake.require({item.get("model_id") for item in catalog.get("device_models", [])} == {
        "device.nxp.tja1050", "device.passive.potentiometer",
        "device.codework_esp.e016.publisher", "device.codework_esp.e016.observer",
    }, "E016 device-model set drift")
    intake.require({item.get("driver_id") for item in catalog.get("driver_contracts", [])} == {
        "driver.codework_esp.e016.publisher", "driver.codework_esp.e016.observer",
    }, "E016 driver-contract set drift")
    board = catalog["board_profiles"][0]
    intake.require(board.get("revision") == "wroom-1", "E016 board revision drift")
    endpoints = {item.get("endpoint_id"): item for item in board.get("endpoints", [])}
    intake.require(set(endpoints) == {"anchor.can0", "gpio1", "gpio10", "gpio11"}, "E016 board endpoint set drift")
    intake.require(endpoints["gpio1"].get("capability_ids") == ["endpoint.analog.input"], "GPIO1 capability drift")
    intake.require(endpoints["gpio10"].get("capability_ids") == ["endpoint.twai.tx"], "GPIO10 capability drift")
    intake.require(endpoints["gpio11"].get("capability_ids") == ["endpoint.twai.rx.protected"], "GPIO11 protection capability drift")

    intake.require(topology.get("schema") == "fisiCs.hardware_topology" and topology.get("project_id") == PROJECT_ID, "E016 topology identity drift")
    intake.require({item.get("node_id") for item in topology.get("nodes", [])} == {"node.bench_a", "node.bench_b"}, "E016 node set drift")
    intake.require(len(topology.get("buses", [])) == 1 and topology["buses"][0].get("scope") == "project_shared", "E016 shared CAN topology drift")
    intake.require(len(topology.get("devices", [])) == 5 and len(topology.get("connections", [])) == 9, "E016 device or connection count drift")
    bindings = {item.get("binding_id"): item for item in topology.get("signal_bindings", [])}
    intake.require(set(bindings) == {"binding.bench_a.e016_consumer", "binding.bench_b.e016_producer"}, "E016 signal binding set drift")
    intake.require(all(item.get("physical_contract_id") == PHYSICAL_CONTRACT_ID for item in bindings.values()), "E016 signal binding physical contract drift")

    expected_manifest = {
        "schema": "fisiCs.hardware_project_manifest",
        "schema_version": 1,
        "project_id": PROJECT_ID,
        "physical_project_graph_path": "../../../build/integration-esp-portable-c/e016.physical-project.graph.json",
        "hardware_catalog_paths": ["e016.hardware.catalog.json"],
        "hardware_topology_path": "e016.hardware.topology.json",
        "physical_evidence_policy": "allow_declared",
        "required_port_policy": "require_all",
    }
    intake.require(manifest == expected_manifest, "E016 hardware project manifest drift")


def run(command: list[str], *, label: str) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["FISICS_MAX_PROCS"] = "0"
    completed = subprocess.run(command, cwd=FISICS_ROOT, env=environment, text=True, capture_output=True, check=False)
    if completed.returncode != 0:
        intake.fail(f"{label} failed with exit {completed.returncode}\nstdout:\n{completed.stdout}\nstderr:\n{completed.stderr}")
    return completed


def expect_failure(command: list[str], *, needles: tuple[str, ...], label: str) -> None:
    environment = os.environ.copy()
    environment["FISICS_MAX_PROCS"] = "0"
    completed = subprocess.run(command, cwd=FISICS_ROOT, env=environment, text=True, capture_output=True, check=False)
    intake.require(completed.returncode != 0, f"{label} unexpectedly succeeded")
    intake.require(any(needle in completed.stderr for needle in needles), f"{label} emitted an unexpected diagnostic: {completed.stderr}")


def verify_hardware_graph(path: Path, contract: dict[str, Any]) -> dict[str, Any]:
    graph = intake.load_json(path)
    expected = contract["expected_graph"]
    intake.require(graph.get("schema") == "fisiCs.hardware_project_graph" and graph.get("complete") is True, "E016 hardware graph identity/completeness drift")
    intake.require(graph.get("project", {}).get("id") == PROJECT_ID, "E016 hardware graph project ID drift")
    counts = {
        "bus_profile_count": len(graph.get("bus_profiles", [])),
        "board_profile_count": len(graph.get("board_profiles", [])),
        "device_model_count": len(graph.get("device_models", [])),
        "driver_contract_count": len(graph.get("driver_contracts", [])),
        "node_count": len(graph.get("nodes", [])),
        "bus_count": len(graph.get("buses", [])),
        "device_count": len(graph.get("devices", [])),
        "connection_count": len(graph.get("connections", [])),
        "signal_binding_count": len(graph.get("signal_bindings", [])),
    }
    for key, actual in counts.items():
        intake.require(actual == expected[key], f"E016 hardware graph {key} drift")
    assessment = graph.get("assessment", {})
    for key in (
        "declared_topology", "hardware_presence", "wiring", "driver_implementation",
        "calibration", "runtime_observation", "deployment", "physical_acceptance",
    ):
        intake.require(assessment.get(key) == expected[key], f"E016 hardware assessment {key} drift")
    intake.require(graph.get("coverage") == {"unbound_required_ports": 0}, "E016 hardware required-port coverage drift")
    bindings = {item.get("binding_id"): item for item in graph.get("signal_bindings", [])}
    intake.require(set(bindings) == {contract["declared_scope"]["consumer_binding"], contract["declared_scope"]["producer_binding"]}, "E016 output signal-binding set drift")
    for item in bindings.values():
        intake.require(item.get("physical_contract_id") == PHYSICAL_CONTRACT_ID, "E016 output physical-contract binding drift")
        intake.require(item.get("physical_evidence_state") == expected["physical_evidence_state"], "E016 physical evidence state was promoted")
        intake.require(item.get("compatibility_mode") == "exact_quantity" and item.get("result") == "compatible", "E016 signal binding compatibility drift")
    return {**counts, "sha256": intake.sha256_path(path), "assessment": assessment, "coverage": graph["coverage"]}


def write_mutated_case(
    temporary: Path,
    authored: dict[str, Path],
    physical_graph: Path,
    name: str,
    mutate: Callable[[dict[str, Any]], None],
    source_role: str = "hardware_topology",
) -> Path:
    topology = intake.load_json(authored["hardware_topology"])
    catalog = intake.load_json(authored["hardware_component_catalog"])
    target_document = topology if source_role == "hardware_topology" else catalog
    mutate(target_document)
    topology_path = temporary / f"{name}.topology.json"
    catalog_path = temporary / f"{name}.catalog.json"
    intake.atomic_write(topology_path, (json.dumps(topology, indent=2) + "\n").encode())
    intake.atomic_write(catalog_path, (json.dumps(catalog, indent=2) + "\n").encode())
    manifest = intake.load_json(authored["hardware_project_manifest"])
    manifest["physical_project_graph_path"] = str(physical_graph)
    manifest["hardware_catalog_paths"] = [str(catalog_path)]
    manifest["hardware_topology_path"] = str(topology_path)
    manifest_path = temporary / f"{name}.manifest.json"
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
    intake.require(contract_path == DEFAULT_CONTRACT.resolve(), "the runner accepts only the fixed Slice D contract")
    intake.require(build_root == DEFAULT_BUILD_ROOT.resolve(), "the runner accepts only the fixed Slice D build root")
    intake.require(compiler.is_file() and os.access(compiler, os.X_OK), f"compiler is not executable: {compiler}")
    intake.require((esp_root / ".git").exists(), f"ESP_WORK_ROOT is not a Git checkout: {esp_root}")
    git_root = intake.git(esp_root, "rev-parse", "--show-toplevel", label="ESP Git root").strip()
    intake.require(Path(git_root).resolve() == esp_root, f"ESP_WORK_ROOT is not the exact Git top level: {esp_root}")
    before_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status before Slice D")
    before_fingerprint, before_entry_count = intake.worktree_fingerprint(esp_root)

    contract = load_hardware_contract(contract_path)
    external = verify_pinned_files(esp_root, contract["external_inputs"], "external hardware authority")
    verify_external_authority(external)
    authored = verify_pinned_files(FISICS_ROOT, contract["authored_inputs"], "authored hardware input")
    verify_authored_inputs(authored)
    slice_c_contract = intake.canonical_child(FISICS_ROOT, contract["slice_c"]["contract_path"], "Slice C contract")
    intake.require(intake.sha256_path(slice_c_contract) == contract["slice_c"]["contract_sha256"], "Slice C contract digest drift")

    run([
        sys.executable, str(FISICS_ROOT / "tests/integration/run_esp_semantic_physical.py"),
        "--esp-work-root", str(esp_root), "--compiler", str(compiler),
        "--contract", str(slice_c_contract), "--build-root", str(build_root),
    ], label="Slice C prerequisite regeneration")
    physical_graph = intake.canonical_child(FISICS_ROOT, contract["slice_c"]["physical_graph_path"], "Slice C physical graph")
    intake.require(intake.sha256_path(physical_graph) == contract["slice_c"]["physical_graph_sha256"], "Slice C physical graph digest drift")

    with tempfile.TemporaryDirectory(prefix="fisics-e016-hardware-") as temporary_name:
        temporary = Path(temporary_name)
        graph_one = temporary / "hardware.one.json"
        graph_two = temporary / "hardware.two.json"
        command = [str(compiler), "--hardware-project-manifest", str(authored["hardware_project_manifest"]), "--emit-hardware-project-graph-json"]
        run([*command, str(graph_one)], label="E016 hardware graph run 1")
        run([*command, str(graph_two)], label="E016 hardware graph run 2")
        intake.require(graph_one.read_bytes() == graph_two.read_bytes(), "E016 hardware graph is not byte deterministic")
        hardware_receipt = verify_hardware_graph(graph_one, contract)
        intake.atomic_write(build_root / "e016.hardware-project.graph.json", graph_one.read_bytes())

        cases = [
            (
                "wrong-board",
                lambda data: data["nodes"][0]["board_profile"].update({"revision": "wrong"}),
                ("unresolved board profile",),
            ),
            (
                "wrong-device",
                lambda data: data["devices"][0]["model"].update({"revision": "wrong"}),
                ("unresolved owner or model",),
            ),
            (
                "wrong-driver",
                lambda data: data["devices"][0]["driver"].update({"driver_id": "driver.missing"}),
                ("unresolved driver contract",),
            ),
            (
                "wrong-bus",
                lambda data: data["buses"][0]["bus_profile"].update({"revision": "wrong"}),
                ("unresolved bus", "unresolved owner or bus profile", "unresolved owner or profile for bus"),
            ),
            (
                "wrong-endpoint",
                lambda data: next(item for item in data["connections"] if item["connection_id"] == "connection.bench_b.tja1050.txd")["target"].update({"board_endpoint_id": "gpio1"}),
                ("capability", "exclusive endpoint conflict"),
            ),
            (
                "missing-allocation",
                lambda data: next(item for item in data["connections"] if item["connection_id"] == "connection.bench_b.publisher.can").update({"allocation_claims": []}),
                ("required allocation",),
            ),
            (
                "missing-binding",
                lambda data: data["signal_bindings"].pop(),
                ("unbound required physical port",),
            ),
            (
                "wrong-physical-contract",
                lambda data: data["signal_bindings"][0].update({"physical_contract_id": "contract.missing"}),
                ("missing physical contract",),
            ),
        ]
        for name, mutate, needles in cases:
            manifest = write_mutated_case(temporary, authored, physical_graph, name, mutate)
            expect_failure([
                str(compiler), "--hardware-project-manifest", str(manifest),
                "--emit-hardware-project-graph-json", str(temporary / f"{name}.graph.json"),
            ], needles=needles, label=f"E016 {name} negative")

    external_after = verify_pinned_files(esp_root, contract["external_inputs"], "external hardware authority")
    intake.require(set(external_after) == set(external), "external hardware input selection changed during Slice D")
    after_fingerprint, after_entry_count = intake.worktree_fingerprint(esp_root)
    after_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status after Slice D")
    intake.require(after_fingerprint == before_fingerprint, "ESP worktree bytes changed during Slice D")
    intake.require(after_entry_count == before_entry_count, "ESP worktree entry count changed during Slice D")
    intake.require(after_status == before_status, "ESP Git status changed during Slice D")

    receipt = {
        "schema": "fisiCs.esp_hardware_compatibility_receipt",
        "schema_version": 1,
        "lane_id": contract["lane_id"],
        "status": "pass",
        "contract": {"path": str(contract_path), "sha256": intake.sha256_path(contract_path)},
        "slice_c": {"contract_sha256": intake.sha256_path(slice_c_contract), "physical_graph_sha256": intake.sha256_path(physical_graph)},
        "external_repository": {
            "path": str(esp_root),
            "head": intake.git(esp_root, "rev-parse", "HEAD", label="ESP HEAD").strip(),
            "status_sha256": intake.sha256_bytes(before_status.encode()),
            "worktree_entry_count": before_entry_count,
            "worktree_fingerprint_sha256": before_fingerprint,
            "unchanged_during_slice_d": True,
        },
        "authored_inputs": contract["authored_inputs"],
        "declared_scope": contract["declared_scope"],
        "hardware_graph": hardware_receipt,
        "negative_proof": {
            "wrong_board_revision": "rejected",
            "wrong_device_revision": "rejected",
            "wrong_driver": "rejected",
            "wrong_bus_revision": "rejected",
            "wrong_endpoint_capability": "rejected",
            "missing_allocation": "rejected",
            "missing_required_binding": "rejected",
            "missing_physical_contract": "rejected",
        },
        "claim_boundary": hardware_receipt["assessment"],
    }
    intake.atomic_write(build_root / "e016.hardware-compatibility.receipt.json", (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode())
    print("E016 Slice D hardware=PASS nodes=2 devices=5 connections=9 bindings=2 deterministic=PASS negatives=PASS live_claims=not_assessed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"E016 Slice D: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
