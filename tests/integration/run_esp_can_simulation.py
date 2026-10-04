#!/usr/bin/env python3
"""Execute ESP-owned sensor models and project their records over Pass 2 CAN intent."""

from __future__ import annotations

import argparse
import copy
from fractions import Fraction
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
from typing import Any, Callable


FISICS_ROOT = Path(__file__).resolve().parents[2]
PASS2_FIXTURE = FISICS_ROOT / "tests/fixtures/esp_can_topology_v1"
PASS2_BUILD = FISICS_ROOT / "build/integration-esp-can-topology"
DEFAULT_FIXTURE_ROOT = FISICS_ROOT / "tests/fixtures/esp_can_simulation_v1"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-can-simulation"


class SimulationError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SimulationError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SimulationError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root must be an object: {path}")
    return value


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def canonical_write(path: Path, value: Any) -> None:
    path.write_bytes(canonical_bytes(value))


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def command(arguments: list[str], *, cwd: Path, label: str) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(arguments, cwd=cwd, env=os.environ.copy(), text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise SimulationError(f"{label} failed ({result.returncode}):\n{result.stdout}")
    return result


def git(root: Path, *arguments: str) -> str:
    return command(["git", "-C", str(root), *arguments], cwd=FISICS_ROOT,
                   label=f"git {' '.join(arguments)}").stdout


def load_contract(path: Path) -> dict[str, Any]:
    value = load_json(path)
    require(set(value) == {"schema", "schema_version", "project_id", "static_graphs",
                           "transport_model", "expected_projection_sha256", "profiles", "routes"},
            "unexpected Pass 3 contract fields")
    require(value["schema"] == "fisiCs.esp_can_simulation_contract",
            "unsupported Pass 3 contract schema")
    require(value["schema_version"] == 1, "unsupported Pass 3 contract version")
    require(value["project_id"] == "esp.can_pass2.sensor_backbone", "Pass 3 project drift")
    require(value["transport_model"] == {
        "bus_id": "bus.can.sensor_backbone", "bitrate": 500000,
        "kind": "deterministic_semantic_delivery_projection",
        "arbitration": "not_modeled", "electrical_behavior": "not_modeled",
    }, "Pass 3 transport-model drift")
    require({item["profile_id"] for item in value["profiles"]} == {
        "environment.aht20.v1", "distance.ultrasonic.v1"
    }, "Pass 3 profile set drift")
    require({item["semantic_id"] for item in value["routes"]} == {
        "signal.bench.ultrasonic.distance",
        "signal.bench.environment.air_temperature",
        "signal.bench.environment.relative_humidity",
    }, "Pass 3 route set drift")
    return value


def run_pass2(esp_root: Path, compiler: Path) -> dict[str, Any]:
    command([
        "python3", str(FISICS_ROOT / "tests/integration/run_esp_can_topology.py"),
        "--esp-work-root", str(esp_root), "--compiler", str(compiler),
        "--fixture-root", str(PASS2_FIXTURE), "--build-root", str(PASS2_BUILD),
    ], cwd=FISICS_ROOT, label="Pass 2 static topology prerequisite")
    return load_json(PASS2_BUILD / "receipt.json")


def validate_static_binding(contract: dict[str, Any], receipt: dict[str, Any]) -> None:
    require(receipt.get("status") == "pass", "Pass 2 receipt is not passing")
    require(receipt.get("project_id") == contract["project_id"], "Pass 2 project mismatch")
    actual = {kind: item["sha256"] for kind, item in receipt["graphs"].items()}
    require(actual == contract["static_graphs"], "exact Pass 2 static graph hash mismatch")

    semantic = load_json(PASS2_BUILD / "semantic.graph.json")
    identities = [item["source_identity"] for item in semantic["records"]]
    graph_edges = {
        (identities[item["source_record_index"]], item["kind"], item["target_id"])
        for item in semantic["edges"]
    }
    for route in contract["routes"]:
        require((route["declared_provider"], "produces", route["semantic_id"]) in graph_edges,
                f"declared producer edge missing for {route['semantic_id']}")
        require((route["declared_consumer"], "consumes", route["semantic_id"]) in graph_edges,
                f"declared consumer edge missing for {route['semantic_id']}")

    hardware = load_json(PASS2_BUILD / "hardware.graph.json")
    nodes = {item["node_id"] for item in hardware["nodes"]}
    buses = {item["bus_id"]: item for item in hardware["buses"]}
    require(contract["transport_model"]["bus_id"] in buses, "declared CAN bus is absent")
    require(buses[contract["transport_model"]["bus_id"]]["bus_profile"] == {
        "profile_id": "bus.can.classical.kbps500", "revision": "pass2-v1"
    }, "declared CAN bus profile mismatch")
    for route in contract["routes"]:
        require(route["producer_node"] in nodes and route["consumer_node"] in nodes,
                f"route node missing for {route['semantic_id']}")


def execute_profile_twice(esp_root: Path, profile: dict[str, Any], temp_root: Path,
                          build_root: Path) -> tuple[dict[str, Any], dict[str, Any]]:
    profile_id = profile["profile_id"]
    request = {
        "schema": "esp_host_request_v2", "profile": profile_id,
        "configuration": {"scenario": profile["scenario"]},
    }
    run_payloads: list[bytes] = []
    plans: list[bytes] = []
    executions: list[dict[str, Any]] = []
    env = os.environ.copy()
    env["PYTHONPATH"] = str(esp_root / "simulation")
    safe_name = profile_id.replace(".", "-")
    for repeat in (1, 2):
        request_path = temp_root / f"{safe_name}.{repeat}.request.json"
        plan_path = temp_root / f"{safe_name}.{repeat}.plan.json"
        output_path = temp_root / f"{safe_name}.{repeat}.run"
        canonical_write(request_path, request)
        result = subprocess.run([
            "python3", "-B", "-m", "control_data", "plan", str(request_path),
            "--out", str(plan_path),
        ], cwd=esp_root, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            raise SimulationError(f"plan {profile_id} repeat {repeat} failed:\n{result.stdout}")
        result = subprocess.run([
            "python3", "-B", "-m", "control_data", "run", str(plan_path),
            "--out", str(output_path),
        ], cwd=esp_root, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            raise SimulationError(f"run {profile_id} repeat {repeat} failed:\n{result.stdout}")
        execution = load_json(output_path / "execution.json")
        require(execution.get("state") == "completed", f"{profile_id} did not complete")
        require(execution.get("realization") == "host_simulation",
                f"{profile_id} realization is not host simulation")
        require(execution.get("physical_hardware") is False,
                f"{profile_id} improperly claims physical hardware")
        run_path = output_path / "result/run.json"
        require(execution["artifacts"].get("run.json") == sha256_path(run_path),
                f"{profile_id} execution artifact join failed")
        plans.append(plan_path.read_bytes())
        run_payloads.append(run_path.read_bytes())
        executions.append(execution)

    require(plans[0] == plans[1], f"{profile_id} plan is not byte deterministic")
    require(run_payloads[0] == run_payloads[1], f"{profile_id} run is not byte deterministic")
    run_hash = sha256_bytes(run_payloads[0])
    require(run_hash == profile["expected_run_sha256"], f"{profile_id} run payload drift")
    retained_path = build_root / f"{safe_name}.run.json"
    retained_path.write_bytes(run_payloads[0])
    return json.loads(run_payloads[0]), {
        "profile_id": profile_id,
        "scenario": profile["scenario"],
        "request": request,
        "plan_sha256": sha256_bytes(plans[0]),
        "run_path": str(retained_path),
        "run_sha256": run_hash,
        "repeat_run_sha256": sha256_bytes(run_payloads[1]),
        "plan_byte_deterministic": True,
        "run_byte_deterministic": True,
        "executions_completed": 2,
        "source_execution_artifact_names": sorted(executions[0]["artifacts"]),
        "source_execution_evidence_verified_twice": True,
        "physical_hardware": False,
    }


def physical_value(raw: int, route: dict[str, Any]) -> dict[str, Any]:
    value = (Fraction(raw * route["scale_numerator"], route["scale_denominator"])
             + Fraction(route["offset_numerator"], route["offset_denominator"]))
    return {"numerator": value.numerator, "denominator": value.denominator,
            "unit": route["physical_unit"]}


def make_record(route: dict[str, Any], profile_hash: str, source_index: int, at_us: int,
                sequence: int, raw: int, validity: int, quality: int,
                fresh: bool, usable: bool, session: int | None) -> dict[str, Any]:
    return {
        "at_us": at_us,
        "source_record_index": source_index,
        "source_profile": route["source_profile"],
        "source_run_sha256": profile_hash,
        "semantic_type": route["semantic_type"],
        "instance": route["instance"],
        "semantic_id": route["semantic_id"],
        "declared_provider": route["declared_provider"],
        "producer_node": route["producer_node"],
        "bus_id": "bus.can.sensor_backbone",
        "declared_consumer": route["declared_consumer"],
        "consumer_node": route["consumer_node"],
        "sequence": sequence,
        "session": session,
        "raw_value": raw,
        "physical_value": physical_value(raw, route),
        "validity": validity,
        "quality": quality,
        "fresh": fresh,
        "usable": usable,
        "transport_state": "delivered",
    }


def project_records(contract: dict[str, Any], runs: dict[str, dict[str, Any]]) -> dict[str, Any]:
    routes = {item["semantic_id"]: item for item in contract["routes"]}
    profile_hashes = {item["profile_id"]: item["expected_run_sha256"]
                      for item in contract["profiles"]}
    records: list[dict[str, Any]] = []

    environment = runs["environment.aht20.v1"]
    require(environment.get("family") == "aht20", "AHT20 result family drift")
    for index, item in enumerate(environment["records"]):
        state = item["state"]
        selected = [signal for signal in state["signals"] if signal.get("instance") == 12]
        require(len(selected) == 1 and selected[0].get("sensor") == "rear_left",
                f"AHT20 rear-left source missing at record {index}")
        signal = selected[0]
        for semantic_id, field in (
            ("signal.bench.environment.air_temperature", "temperature"),
            ("signal.bench.environment.relative_humidity", "humidity"),
        ):
            sample = signal[field]
            records.append(make_record(
                routes[semantic_id], profile_hashes["environment.aht20.v1"], index,
                item["at_us"], sample["sequence"], sample["raw"], sample["validity"],
                sample["quality"], item["at_us"] // 1000 == sample["received_ms"],
                sample["validity"] == 0 and sample["quality"] > 0,
                sample["session"],
            ))

    ultrasonic = runs["distance.ultrasonic.v1"]
    require(ultrasonic.get("family") == "ultrasonic", "ultrasonic result family drift")
    route = routes["signal.bench.ultrasonic.distance"]
    for index, item in enumerate(ultrasonic["records"]):
        state = item["state"]
        records.append(make_record(
            route, profile_hashes["distance.ultrasonic.v1"], index, item["at_us"],
            state["step"], state["raw_mm"], 0 if state["valid"] else 1,
            1 if state["valid"] else 0, bool(state["fresh"]), bool(state["usable"]), None,
        ))

    records.sort(key=lambda item: (item["at_us"], item["semantic_id"], item["source_record_index"]))
    edges = []
    for route in sorted(contract["routes"], key=lambda item: item["semantic_id"]):
        edge_records = [item for item in records if item["semantic_id"] == route["semantic_id"]]
        edges.append({
            "semantic_id": route["semantic_id"],
            "declared_provider": route["declared_provider"],
            "producer_node": route["producer_node"],
            "declared_consumer": route["declared_consumer"],
            "consumer_node": route["consumer_node"],
            "bus_id": contract["transport_model"]["bus_id"],
            "delivered_records": len(edge_records),
            "usable_records": sum(1 for item in edge_records if item["usable"]),
            "state": "simulated_observed",
        })
    return {
        "schema": "fisiCs.esp_can_simulated_observation",
        "schema_version": 1,
        "project_id": contract["project_id"],
        "static_graphs": contract["static_graphs"],
        "transport_model": contract["transport_model"],
        "records": records,
        "observed_edges": edges,
    }


def validate_projection(contract: dict[str, Any], projection: dict[str, Any]) -> None:
    require(projection.get("static_graphs") == contract["static_graphs"],
            "projection static graph hash mismatch")
    require(projection.get("transport_model") == contract["transport_model"],
            "projection transport model mismatch")
    routes = {item["semantic_id"]: item for item in contract["routes"]}
    expected_profiles = {item["profile_id"]: item["expected_run_sha256"]
                         for item in contract["profiles"]}
    last_sequences: dict[str, int] = {}
    for record in projection["records"]:
        semantic_id = record["semantic_id"]
        require(semantic_id in routes, f"unmapped simulated semantic identity: {semantic_id}")
        route = routes[semantic_id]
        for key in ("semantic_type", "instance", "declared_provider", "producer_node",
                    "declared_consumer", "consumer_node", "source_profile"):
            require(record[key] == route[key], f"simulated route {key} mismatch for {semantic_id}")
        require(record["bus_id"] == contract["transport_model"]["bus_id"],
                f"simulated bus mismatch for {semantic_id}")
        require(record["transport_state"] == "delivered", "non-delivered record in projection")
        require(record["source_run_sha256"] == expected_profiles[record["source_profile"]],
                f"source run mismatch for {semantic_id}")
        require(record["physical_value"] == physical_value(record["raw_value"], route),
                f"physical conversion mismatch for {semantic_id}")
        if semantic_id in last_sequences:
            require(record["sequence"] >= last_sequences[semantic_id],
                    f"sequence regression for {semantic_id}")
        last_sequences[semantic_id] = record["sequence"]

    actual_ids = {item["semantic_id"] for item in projection["records"]}
    require(actual_ids == set(routes), "simulated signal coverage mismatch")
    expected_edges = {
        (item["semantic_id"], item["declared_provider"], item["producer_node"],
         item["declared_consumer"], item["consumer_node"], "simulated_observed")
        for item in contract["routes"]
    }
    actual_edges = {
        (item["semantic_id"], item["declared_provider"], item["producer_node"],
         item["declared_consumer"], item["consumer_node"], item["state"])
        for item in projection["observed_edges"]
    }
    require(actual_edges == expected_edges, "simulated observed edge coverage mismatch")


def expect_failure(contract: dict[str, Any], projection: dict[str, Any],
                   mutate: Callable[[dict[str, Any]], None], label: str) -> str:
    candidate = copy.deepcopy(projection)
    mutate(candidate)
    try:
        validate_projection(contract, candidate)
    except (SimulationError, KeyError):
        return label
    raise SimulationError(f"negative projection unexpectedly passed: {label}")


def negative_proof(contract: dict[str, Any], projection: dict[str, Any]) -> list[str]:
    first = projection["records"][0]
    first_signal = first["semantic_id"]
    same_signal = [index for index, item in enumerate(projection["records"])
                   if item["semantic_id"] == first_signal]
    require(len(same_signal) >= 2, "insufficient records for sequence negative")
    return [
        expect_failure(contract, projection,
                       lambda value: value["static_graphs"].__setitem__("hardware", "0" * 64),
                       "wrong_static_graph_hash_rejected"),
        expect_failure(contract, projection,
                       lambda value: value["records"][0].__setitem__("semantic_id", "signal.unknown"),
                       "unmapped_semantic_identity_rejected"),
        expect_failure(contract, projection,
                       lambda value: value["records"][0].__setitem__("producer_node", "node.undeclared"),
                       "undeclared_publisher_node_rejected"),
        expect_failure(contract, projection,
                       lambda value: value["records"][same_signal[1]].__setitem__(
                           "sequence", value["records"][same_signal[0]]["sequence"] - 1),
                       "sequence_regression_rejected"),
        expect_failure(contract, projection,
                       lambda value: value["records"][0]["physical_value"].__setitem__("numerator", 999),
                       "physical_conversion_mismatch_rejected"),
        expect_failure(contract, projection,
                       lambda value: value["observed_edges"][0].__setitem__(
                           "consumer_node", "node.undeclared"),
                       "undeclared_consumer_node_rejected"),
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
    require(fixture_root == DEFAULT_FIXTURE_ROOT.resolve(), "runner accepts only fixed Pass 3 fixture")
    require(build_root == DEFAULT_BUILD_ROOT.resolve(), "runner accepts only fixed Pass 3 build root")
    require(compiler.is_file() and os.access(compiler, os.X_OK), "compiler is not executable")
    require((esp_root / ".git").exists(), "ESP root is not a Git checkout")
    require(Path(git(esp_root, "rev-parse", "--show-toplevel").strip()).resolve() == esp_root,
            "ESP root is not the exact Git top level")

    before_status = git(esp_root, "status", "--porcelain=v2", "--branch")
    contract = load_contract(fixture_root / "contract.json")
    pass2_receipt = run_pass2(esp_root, compiler)
    validate_static_binding(contract, pass2_receipt)
    build_root.mkdir(parents=True, exist_ok=True)

    runs: dict[str, dict[str, Any]] = {}
    profile_receipts: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="fisics-esp-can-simulation-") as temp:
        for profile in contract["profiles"]:
            payload, receipt = execute_profile_twice(
                esp_root, profile, Path(temp), build_root)
            runs[profile["profile_id"]] = payload
            profile_receipts.append(receipt)

    projection = project_records(contract, runs)
    validate_projection(contract, projection)
    negatives = negative_proof(contract, projection)
    projection_path = build_root / "simulated-observation.json"
    canonical_write(projection_path, projection)
    projection_repeat = project_records(contract, runs)
    require(canonical_bytes(projection) == canonical_bytes(projection_repeat),
            "simulated projection is not byte deterministic")
    require(sha256_path(projection_path) == contract["expected_projection_sha256"],
            "simulated projection payload drift")

    after_status = git(esp_root, "status", "--porcelain=v2", "--branch")
    require(after_status == before_status, "ESP Git status changed during Pass 3")
    records = projection["records"]
    receipt = {
        "schema": "fisiCs.esp_can_simulation_receipt",
        "schema_version": 1,
        "project_id": contract["project_id"],
        "status": "pass",
        "contract": {"path": str(fixture_root / "contract.json"),
                     "sha256": sha256_path(fixture_root / "contract.json")},
        "static_graph_receipt": {
            "path": str(PASS2_BUILD / "receipt.json"),
            "sha256": sha256_path(PASS2_BUILD / "receipt.json"),
            "graphs": contract["static_graphs"],
        },
        "source_simulations": profile_receipts,
        "simulated_observation": {
            "path": str(projection_path), "sha256": sha256_path(projection_path),
            "byte_deterministic": True,
            "records": len(records), "usable_records": sum(1 for item in records if item["usable"]),
            "invalid_or_stale_records": sum(1 for item in records if not item["usable"]),
            "observed_edges": len(projection["observed_edges"]),
        },
        "negative_proof": negatives,
        "claim_boundary": {
            "semantic_metadata": "compiler_validated",
            "physical_composition": "statically_compatible",
            "hardware_topology": "statically_compatible",
            "sensor_models": "host_simulation_executed",
            "can_transport": "deterministic_semantic_delivery_projection",
            "simulated_observed_projection": "validated",
            "can_arbitration": "not_modeled",
            "electrical_behavior": "not_assessed",
            "runtime_hardware_observation": "not_assessed",
            "firmware_build": "not_assessed",
            "deployment": "not_assessed",
            "wiring": "not_assessed",
            "physical_acceptance": "not_assessed"
        },
        "external_repository": {
            "path": str(esp_root), "head": git(esp_root, "rev-parse", "HEAD").strip(),
            "status_sha256": hashlib.sha256(before_status.encode()).hexdigest(),
            "git_status_unchanged": True,
        },
    }
    first = canonical_bytes(receipt)
    require(first == canonical_bytes(receipt), "Pass 3 receipt serialization is not deterministic")
    (build_root / "receipt.json").write_bytes(first)
    print(f"ESP CAN Pass 3=PASS profiles=2 executions=4 records={len(records)} "
          f"edges=3 negatives={len(negatives)} external_tree=UNCHANGED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except SimulationError as exc:
        print(f"ESP CAN Pass 3: FAIL: {exc}", file=os.sys.stderr)
        raise SystemExit(1)
