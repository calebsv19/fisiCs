#!/usr/bin/env python3
"""Build the bounded Pass 2 ESP sensor/CAN static topology projection."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from typing import Any, Callable


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_FIXTURE_ROOT = FISICS_ROOT / "tests/fixtures/esp_can_topology_v1"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-can-topology"


class TopologyError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise TopologyError(message)


def sha256_path(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TopologyError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root must be an object: {path}")
    return value


def canonical_write(path: Path, value: dict[str, Any]) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def run(command: list[str], *, label: str, expect_success: bool = True) -> subprocess.CompletedProcess[str]:
    env = os.environ.copy()
    env["FISICS_MAX_PROCS"] = "0"
    result = subprocess.run(command, cwd=FISICS_ROOT, env=env, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if expect_success and result.returncode != 0:
        raise TopologyError(f"{label} failed ({result.returncode}):\n{result.stdout}")
    if not expect_success and result.returncode == 0:
        raise TopologyError(f"{label} unexpectedly succeeded")
    return result


def git(root: Path, *args: str) -> str:
    result = run(["git", "-C", str(root), *args], label=f"git {' '.join(args)}")
    return result.stdout


def load_contract(path: Path) -> dict[str, Any]:
    data = load_json(path)
    require(set(data) == {
        "schema", "schema_version", "project_id", "external_inputs",
        "profile_expectations", "protocol_expectations", "semantic_mappings",
        "topology_expectations",
    }, "unexpected Pass 2 external contract fields")
    require(data["schema"] == "fisiCs.esp_can_topology_external_contract",
            "unsupported Pass 2 external contract schema")
    require(data["schema_version"] == 1, "unsupported Pass 2 external contract version")
    require(data["project_id"] == "esp.can_pass2.sensor_backbone", "Pass 2 project identity drift")
    return data


def verify_external_inputs(esp_root: Path, contract: dict[str, Any]) -> list[dict[str, str]]:
    receipts: list[dict[str, str]] = []
    role_paths: dict[str, Path] = {}
    for item in contract["external_inputs"]:
        require(set(item) == {"role", "path", "sha256"}, "unexpected external input fields")
        path = (esp_root / item["path"]).resolve()
        require(path.is_relative_to(esp_root), f"external input escapes ESP root: {item['path']}")
        require(path.is_file(), f"missing external input: {item['path']}")
        actual = sha256_path(path)
        require(actual == item["sha256"], f"external input digest mismatch for {item['role']}")
        require(item["role"] not in role_paths, f"duplicate external role: {item['role']}")
        role_paths[item["role"]] = path
        receipts.append({"role": item["role"], "path": item["path"], "sha256": actual})

    for expected in contract["profile_expectations"]:
        profile = load_json(role_paths[expected["role"]])
        for key in ("id", "adapter", "state"):
            require(profile.get(key) == expected[key],
                    f"{expected['role']} {key} does not match Pass 2 contract")
        require(profile.get("fixed", {}).get("physical_hardware") is False,
                f"{expected['role']} improperly claims physical hardware")
        require(profile.get("protocol_sha256") == contract["protocol_expectations"]["schema_identity_sha256"],
                f"{expected['role']} protocol identity does not match Pass 2 contract")

    protocol_text = role_paths["protocol_schema"].read_text(encoding="utf-8")
    for name in contract["protocol_expectations"]["semantic_types"]:
        require(f'"name": "{name}"' in protocol_text, f"protocol semantic type missing: {name}")
    for name in contract["protocol_expectations"]["instances"]:
        require(f'"name": "{name}"' in protocol_text, f"protocol instance missing: {name}")

    expected_mapping_fields = {
        "semantic_type", "instance", "canonical_semantic_identity", "physical_contract_id",
        "wire_unit", "physical_unit", "scale_numerator", "scale_denominator",
        "offset_numerator", "offset_denominator",
    }
    semantic_ids: set[str] = set()
    physical_contract_ids: set[str] = set()
    for mapping in contract["semantic_mappings"]:
        require(set(mapping) == expected_mapping_fields, "unexpected semantic mapping fields")
        require(mapping["semantic_type"] in contract["protocol_expectations"]["semantic_types"],
                f"mapping uses undeclared semantic type: {mapping['semantic_type']}")
        require(mapping["instance"] in contract["protocol_expectations"]["instances"],
                f"mapping uses undeclared instance: {mapping['instance']}")
        require(mapping["scale_denominator"] > 0 and mapping["offset_denominator"] > 0,
                "semantic mapping denominators must be positive")
        require(mapping["canonical_semantic_identity"] not in semantic_ids,
                "duplicate canonical semantic identity mapping")
        semantic_ids.add(mapping["canonical_semantic_identity"])
        physical_contract_ids.add(mapping["physical_contract_id"])
    require(semantic_ids == {
        "signal.bench.ultrasonic.distance",
        "signal.bench.environment.air_temperature",
        "signal.bench.environment.relative_humidity",
    }, "semantic mapping identity coverage drift")
    require(physical_contract_ids == {
        "contract.bench.ultrasonic.distance",
        "contract.bench.environment.air_temperature",
        "contract.bench.environment.relative_humidity",
    }, "semantic mapping physical contract coverage drift")
    return receipts


def validate_topology_intent(
    catalog: dict[str, Any], topology: dict[str, Any], expectations: dict[str, Any]
) -> None:
    require(len(topology["nodes"]) == expectations["nodes"], "Pass 2 node count drift")
    require(len(topology["buses"]) == expectations["buses"], "Pass 2 bus count drift")
    require(len(topology["devices"]) == expectations["devices"], "Pass 2 device count drift")
    require(len(topology["connections"]) == expectations["connections"], "Pass 2 connection count drift")
    require(len(topology["signal_bindings"]) == expectations["signal_bindings"], "Pass 2 signal binding count drift")

    profiles = {
        (item["profile_id"], item["revision"]): set(item["capability_ids"])
        for item in catalog["board_profiles"]
    }
    endpoint_nodes: list[str] = []
    middle_nodes: list[str] = []
    for node in topology["nodes"]:
        key = (node["board_profile"]["profile_id"], node["board_profile"]["revision"])
        require(key in profiles, f"unknown board profile in topology intent: {key}")
        capabilities = profiles[key]
        if "topology.can.endpoint" in capabilities:
            endpoint_nodes.append(node["node_id"])
        if "topology.can.middle" in capabilities:
            middle_nodes.append(node["node_id"])
    require(sorted(endpoint_nodes) == sorted(expectations["endpoint_nodes"]), "CAN endpoint role drift")
    require(sorted(middle_nodes) == sorted(expectations["middle_nodes"]), "CAN middle-node role drift")
    require(set(endpoint_nodes).isdisjoint(middle_nodes), "a CAN node cannot be both endpoint and middle")

    terminations = [
        item for item in topology["devices"]
        if item["model"] == {
            "model_id": "device.passive.can_termination_resistor",
            "revision": "120_ohm_intent-v1",
        }
    ]
    require(sorted(item["device_id"] for item in terminations) == sorted(expectations["termination_devices"]),
            "CAN termination device set drift")
    require(len(terminations) == 2, "Pass 2 requires exactly two authored CAN terminations")
    require({item["owner_node_id"] for item in terminations} == set(endpoint_nodes),
            "CAN terminations must be owned by the two endpoint nodes")
    require(not ({item["owner_node_id"] for item in terminations} & set(middle_nodes)),
            "CAN middle node must not own a termination")

    can_buses = [item for item in topology["buses"] if item["bus_id"] == "bus.can.sensor_backbone"]
    require(len(can_buses) == 1 and can_buses[0]["scope"] == "project_shared",
            "Pass 2 requires one project-shared CAN backbone")
    require(can_buses[0]["bus_profile"] == {
        "profile_id": "bus.can.classical.kbps500", "revision": "pass2-v1"
    }, "Pass 2 CAN profile drift")

    transceiver_owners = {
        item["owner_node_id"] for item in topology["devices"]
        if item["model"]["model_id"] == "device.nxp.tja1050"
    }
    require(transceiver_owners == {item["node_id"] for item in topology["nodes"]},
            "every Pass 2 node must own one declared CAN transceiver")


def emit_graphs(compiler: Path, fixture_root: Path, build_root: Path) -> dict[str, str]:
    build_root.mkdir(parents=True, exist_ok=True)
    for stem in ("signals", "supervisor"):
        run([
            str(compiler), "--overlay=physics-units,semantic-metadata",
            f"-I{FISICS_ROOT / 'include'}",
            "--emit-semantic-metadata-json", str(build_root / f"{stem}.semantic.json"),
            "--emit-units-metadata-json", str(build_root / f"{stem}.units.json"),
            "-c", str(fixture_root / f"{stem}.c"), "-o", str(build_root / f"{stem}.o"),
        ], label=f"compile {stem} metadata carrier")

    canonical_write(build_root / "semantic.manifest.json", {
        "schema": "fisiCs.semantic_project_manifest", "schema_version": 1,
        "project_id": "esp.can_pass2.sensor_backbone",
        "translation_units": [
            {"metadata_path": "signals.semantic.json"},
            {"metadata_path": "supervisor.semantic.json"},
        ],
        "allowed_external_targets": [],
    })
    canonical_write(build_root / "physical.manifest.json", {
        "schema": "fisiCs.physical_project_manifest", "schema_version": 1,
        "project_id": "esp.can_pass2.sensor_backbone",
        "semantic_project_graph_path": "semantic.graph.json",
        "units_metadata_paths": ["signals.units.json", "supervisor.units.json"],
        "physical_catalog_paths": [str(fixture_root / "physical.catalog.json")],
        "coverage_policy": "require_relationship_targets",
        "source_evidence_policy": "report_unverified",
    })
    canonical_write(build_root / "hardware.manifest.json", {
        "schema": "fisiCs.hardware_project_manifest", "schema_version": 1,
        "project_id": "esp.can_pass2.sensor_backbone",
        "physical_project_graph_path": "physical.graph.json",
        "hardware_catalog_paths": [str(fixture_root / "hardware.catalog.json")],
        "hardware_topology_path": str(fixture_root / "hardware.topology.json"),
        "physical_evidence_policy": "require_source_compatible",
        "required_port_policy": "require_all",
    })

    commands = {
        "semantic": ["--semantic-project-manifest", str(build_root / "semantic.manifest.json"),
                     "--emit-semantic-project-graph-json"],
        "physical": ["--physical-project-manifest", str(build_root / "physical.manifest.json"),
                     "--emit-physical-project-graph-json"],
        "hardware": ["--hardware-project-manifest", str(build_root / "hardware.manifest.json"),
                     "--emit-hardware-project-graph-json"],
    }
    hashes: dict[str, str] = {}
    for kind, arguments in commands.items():
        first = build_root / f"{kind}.graph.json"
        second = build_root / f"{kind}.repeat.graph.json"
        run([str(compiler), *arguments, str(first)], label=f"emit {kind} graph")
        run([str(compiler), *arguments, str(second)], label=f"repeat {kind} graph")
        require(first.read_bytes() == second.read_bytes(), f"{kind} graph is not byte deterministic")
        hashes[kind] = sha256_path(first)
    return hashes


def validate_graphs(build_root: Path, expectations: dict[str, Any]) -> dict[str, int]:
    semantic = load_json(build_root / "semantic.graph.json")
    physical = load_json(build_root / "physical.graph.json")
    hardware = load_json(build_root / "hardware.graph.json")
    require(semantic.get("complete") is True, "semantic graph is incomplete")
    require(physical.get("complete") is True, "physical graph is incomplete")
    require(hardware.get("complete") is True, "hardware graph is incomplete")

    expected_signals = {
        "signal.bench.ultrasonic.distance",
        "signal.bench.environment.air_temperature",
        "signal.bench.environment.relative_humidity",
    }
    actual_records = {item["source_identity"] for item in semantic["records"]}
    require(expected_signals <= actual_records, "semantic signal identity coverage drift")
    edges = {(item["kind"], item["target_id"]) for item in semantic["edges"]}
    for target in expected_signals:
        require(("produces", target) in edges and ("consumes", target) in edges,
                f"producer/consumer edge coverage missing for {target}")

    contracts = {item["contract_id"]: item for item in physical["physical_contracts"]}
    require(set(contracts) == {
        "contract.bench.ultrasonic.distance",
        "contract.bench.environment.air_temperature",
        "contract.bench.environment.relative_humidity",
    }, "physical contract set drift")
    require(all(item["state"] == "source_compatible" for item in contracts.values()),
            "Pass 2 physical contracts are not source-compatible")

    require(len(hardware["nodes"]) == expectations["nodes"], "hardware graph node count drift")
    require(len(hardware["buses"]) == expectations["buses"], "hardware graph bus count drift")
    require(len(hardware["devices"]) == expectations["devices"], "hardware graph device count drift")
    require(len(hardware["connections"]) == expectations["connections"], "hardware graph connection count drift")
    require(len(hardware["signal_bindings"]) == expectations["signal_bindings"], "hardware graph binding count drift")
    require(hardware["coverage"]["unbound_required_ports"] == 0, "hardware graph has unbound required ports")
    require(hardware["assessment"]["declared_topology"] == "compatible", "topology is not statically compatible")
    for key in ("hardware_presence", "wiring", "driver_implementation", "calibration",
                "runtime_observation", "simulation", "deployment", "physical_acceptance"):
        require(hardware["assessment"][key] == "not_assessed", f"hardware graph promoted {key}")
    return {"semantic_records": len(semantic["records"]), "semantic_edges": len(semantic["edges"])}


def expect_intent_failure(catalog: dict[str, Any], topology: dict[str, Any],
                          expectations: dict[str, Any], mutate: Callable[[dict[str, Any]], None],
                          label: str) -> None:
    candidate = copy.deepcopy(topology)
    mutate(candidate)
    try:
        validate_topology_intent(catalog, candidate, expectations)
    except TopologyError:
        return
    raise TopologyError(f"intent negative unexpectedly passed: {label}")


def expect_hardware_failure(compiler: Path, build_root: Path, catalog: dict[str, Any],
                            topology: dict[str, Any], mutate_catalog: Callable[[dict[str, Any]], None] | None,
                            mutate_topology: Callable[[dict[str, Any]], None] | None,
                            needle: str, label: str) -> None:
    with tempfile.TemporaryDirectory(prefix="fisics-esp-can-negative-") as temp:
        root = Path(temp)
        candidate_catalog = copy.deepcopy(catalog)
        candidate_topology = copy.deepcopy(topology)
        if mutate_catalog:
            mutate_catalog(candidate_catalog)
        if mutate_topology:
            mutate_topology(candidate_topology)
        canonical_write(root / "catalog.json", candidate_catalog)
        canonical_write(root / "topology.json", candidate_topology)
        canonical_write(root / "manifest.json", {
            "schema": "fisiCs.hardware_project_manifest", "schema_version": 1,
            "project_id": "esp.can_pass2.sensor_backbone",
            "physical_project_graph_path": str(build_root / "physical.graph.json"),
            "hardware_catalog_paths": [str(root / "catalog.json")],
            "hardware_topology_path": str(root / "topology.json"),
            "physical_evidence_policy": "require_source_compatible",
            "required_port_policy": "require_all",
        })
        result = run([
            str(compiler), "--hardware-project-manifest", str(root / "manifest.json"),
            "--emit-hardware-project-graph-json", str(root / "hardware.graph.json"),
        ], label=label, expect_success=False)
        require(needle in result.stdout, f"{label} failed for the wrong reason:\n{result.stdout}")


def run_negative_proof(compiler: Path, fixture_root: Path, build_root: Path,
                       contract: dict[str, Any]) -> list[str]:
    catalog = load_json(fixture_root / "hardware.catalog.json")
    topology = load_json(fixture_root / "hardware.topology.json")
    expectations = contract["topology_expectations"]

    def remove_supervisor_termination(candidate: dict[str, Any]) -> None:
        candidate["devices"] = [item for item in candidate["devices"]
                                if item["device_id"] != "device.can.supervisor_termination"]
        candidate["connections"] = [item for item in candidate["connections"]
                                    if item["device_id"] != "device.can.supervisor_termination"]

    def move_termination_to_middle(candidate: dict[str, Any]) -> None:
        next(item for item in candidate["devices"]
             if item["device_id"] == "device.can.supervisor_termination")["owner_node_id"] = "node.can.environment_middle"

    expect_intent_failure(catalog, topology, expectations, remove_supervisor_termination,
                          "missing endpoint termination")
    expect_intent_failure(catalog, topology, expectations, move_termination_to_middle,
                          "middle-node termination")

    def collide_gpio(candidate: dict[str, Any]) -> None:
        next(item for item in candidate["connections"]
             if item["connection_id"] == "connection.can.environment_tja1050.rxd")["target"]["board_endpoint_id"] = "gpio10"

    def admit_rx_on_gpio10(candidate: dict[str, Any]) -> None:
        profile = next(item for item in candidate["board_profiles"]
                       if item["revision"] == "can-middle-environment-v1")
        endpoint = next(item for item in profile["endpoints"] if item["endpoint_id"] == "gpio10")
        endpoint["capability_ids"].append("endpoint.twai.rx.protected")

    def remove_binding(candidate: dict[str, Any]) -> None:
        candidate["signal_bindings"] = [item for item in candidate["signal_bindings"]
                                        if item["binding_id"] != "binding.can.supervisor.humidity"]

    def mismatch_quantity(candidate: dict[str, Any]) -> None:
        model = next(item for item in candidate["device_models"]
                     if item["model_id"] == "device.sensor.aht20")
        port = next(item for item in model["physical_ports"] if item["port_id"] == "port.temperature")
        port["requirement"]["quantity_kind"] = "quantity.environment.relative_humidity"

    def mismatch_transport(candidate: dict[str, Any]) -> None:
        bus = next(item for item in candidate["bus_profiles"]
                   if item["profile_id"] == "bus.can.classical.kbps500")
        bus["transport_kind"] = "transport.can.classical.kbps250"

    expect_hardware_failure(compiler, build_root, catalog, topology, admit_rx_on_gpio10, collide_gpio,
                            "exclusive direct endpoint conflict",
                            "exclusive GPIO collision negative")
    expect_hardware_failure(compiler, build_root, catalog, topology, None, remove_binding,
                            "unbound required physical port",
                            "missing required physical binding negative")
    expect_hardware_failure(compiler, build_root, catalog, topology, mismatch_quantity, None,
                            "quantity mismatch",
                            "physical quantity mismatch negative")
    expect_hardware_failure(compiler, build_root, catalog, topology, mismatch_transport, None,
                            "transport or participant capability mismatch",
                            "CAN transport mismatch negative")
    return [
        "missing_endpoint_termination_rejected_by_intent_validator",
        "middle_node_termination_rejected_by_intent_validator",
        "exclusive_gpio_collision_rejected_by_hardware_graph",
        "missing_required_physical_binding_rejected_by_hardware_graph",
        "physical_quantity_mismatch_rejected_by_hardware_graph",
        "can_transport_mismatch_rejected_by_hardware_graph",
    ]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--esp-work-root", required=True, type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    parser.add_argument("--fixture-root", type=Path, default=DEFAULT_FIXTURE_ROOT)
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    args = parser.parse_args()

    esp_root = args.esp_work_root.resolve()
    compiler = args.compiler.resolve()
    fixture_root = args.fixture_root.resolve()
    build_root = args.build_root.resolve()
    require(fixture_root == DEFAULT_FIXTURE_ROOT.resolve(), "runner accepts only the fixed Pass 2 fixture root")
    require(build_root == DEFAULT_BUILD_ROOT.resolve(), "runner accepts only the fixed Pass 2 build root")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler is not executable")
    require((esp_root / ".git").exists(), "ESP root is not a Git checkout")
    require(Path(git(esp_root, "rev-parse", "--show-toplevel").strip()).resolve() == esp_root,
            "ESP root is not the exact Git top level")

    before_status = git(esp_root, "status", "--porcelain=v2", "--branch")
    contract = load_contract(fixture_root / "external.contract.json")
    external_receipts = verify_external_inputs(esp_root, contract)
    catalog = load_json(fixture_root / "hardware.catalog.json")
    topology = load_json(fixture_root / "hardware.topology.json")
    validate_topology_intent(catalog, topology, contract["topology_expectations"])

    graph_hashes = emit_graphs(compiler, fixture_root, build_root)
    graph_counts = validate_graphs(build_root, contract["topology_expectations"])
    negative_proof = run_negative_proof(compiler, fixture_root, build_root, contract)

    require(verify_external_inputs(esp_root, contract) == external_receipts,
            "selected ESP external inputs changed during Pass 2")
    after_status = git(esp_root, "status", "--porcelain=v2", "--branch")
    require(after_status == before_status, "ESP Git status changed during Pass 2")

    receipt = {
        "schema": "fisiCs.esp_can_topology_receipt",
        "schema_version": 1,
        "project_id": contract["project_id"],
        "status": "pass",
        "producer": {"name": "fisiCs", "version": "0.7.0"},
        "external_inputs": external_receipts,
        "protocol_projection": {
            "source_sha256": next(item["sha256"] for item in external_receipts
                                  if item["role"] == "protocol_schema"),
            "schema_identity_sha256": contract["protocol_expectations"]["schema_identity_sha256"],
            "selected_profiles_match_schema_identity": True,
            "semantic_mappings": contract["semantic_mappings"],
        },
        "authored_inputs": {
            name: {"path": str(fixture_root / name), "sha256": sha256_path(fixture_root / name)}
            for name in ("signals.c", "supervisor.c", "physical.catalog.json",
                         "hardware.catalog.json", "hardware.topology.json", "external.contract.json")
        },
        "graphs": {
            kind: {"path": str(build_root / f"{kind}.graph.json"), "sha256": digest,
                   "byte_deterministic": True}
            for kind, digest in graph_hashes.items()
        },
        "inventory": {**contract["topology_expectations"], **graph_counts},
        "negative_proof": negative_proof,
        "namespace_migration": {
            "quantity.dc.temperature": "quantity.environment.air_temperature.absolute",
            "quantity.dc.relative_humidity": "quantity.environment.relative_humidity",
            "retained_artifacts_rewritten": False,
        },
        "termination_contract": {
            "representation": "two passive 120-ohm-intent devices on endpoint-role nodes",
            "generic_hardware_graph_cardinality": "not_implemented",
            "bounded_intent_validator": "passed",
            "physical_resistance": "not_assessed",
        },
        "claim_boundary": {
            "semantic_metadata": "compiler_validated",
            "physical_composition": "statically_compatible",
            "hardware_topology": "statically_compatible",
            "simulation": "not_executed",
            "runtime_observation": "not_assessed",
            "deployment": "not_assessed",
            "wiring": "not_assessed",
            "physical_acceptance": "not_assessed",
        },
        "external_repository": {
            "path": str(esp_root),
            "head": git(esp_root, "rev-parse", "HEAD").strip(),
            "status_sha256": hashlib.sha256(before_status.encode()).hexdigest(),
            "selected_inputs_unchanged": True,
            "git_status_unchanged": True,
        },
    }
    first = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode()
    second = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode()
    require(first == second, "Pass 2 receipt serialization is not deterministic")
    (build_root / "receipt.json").write_bytes(first)
    print("ESP CAN Pass 2=PASS nodes=3 buses=2 devices=8 connections=15 bindings=6 negatives=6 external_tree=UNCHANGED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TopologyError as exc:
        print(f"ESP CAN Pass 2: FAIL: {exc}", file=os.sys.stderr)
        raise SystemExit(1)
