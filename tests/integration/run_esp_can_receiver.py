#!/usr/bin/env python3
"""Pass 5 deterministic supervisor receive, conformance, and replay projection."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import sys
from typing import Any, Callable


FISICS_ROOT = Path(__file__).resolve().parents[2]
INTEGRATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(INTEGRATION_ROOT))
import run_esp_can_transport as can_transport  # noqa: E402


DEFAULT_FIXTURE_ROOT = FISICS_ROOT / "tests/fixtures/esp_can_receiver_v1"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-can-receiver"
DEFAULT_TRANSPORT = FISICS_ROOT / "build/integration-esp-can-transport/transport.json"
DEFAULT_PASS4_RECEIPT = FISICS_ROOT / "build/integration-esp-can-transport/receipt.json"


class ReceiverError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ReceiverError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ReceiverError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root must be an object: {path}")
    return value


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def canonical_write(path: Path, value: Any) -> None:
    path.write_bytes(canonical_bytes(value))


def sha256_path(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def validate_contract(contract: dict[str, Any], *, allow_unpinned: bool = False) -> None:
    require(set(contract) == {
        "schema", "schema_version", "project_id", "source_transport", "receiver", "routes",
        "queue_scenarios", "conformance_vectors", "wire_capabilities",
        "expected_receiver_sha256", "expected_replay_sha256",
    }, "unexpected Pass 5 contract fields")
    require(contract["schema"] == "fisiCs.esp_can_receiver_contract",
            "unsupported Pass 5 contract schema")
    require(contract["schema_version"] == 1, "unsupported Pass 5 contract version")
    require(contract["project_id"] == "esp.can_pass2.sensor_backbone", "Pass 5 project drift")
    require(contract["source_transport"] == {
        "schema": "fisiCs.esp_can_transport_projection",
        "sha256": "366c64620d01fb4c38fc327bcef81a3e412728914f80df08954d36b0c2f18437",
        "scenario_id": "baseline_transient_ack_loss", "frames": 149, "deliveries": 149,
    }, "Pass 5 source transport contract drift")
    require(contract["receiver"] == {
        "node_id": "node.can.supervisor_endpoint",
        "payload_codec": "fisiCs.sensor_scalar_le_v1",
        "sequence_policy": "strictly_increasing_per_semantic_identity",
        "latest_good_policy": "valid_and_fresh_and_usable_only",
        "overflow_policy": "drop_newest",
        "service_policy": "single_fifo_fixed_service_time",
    }, "Pass 5 receiver policy drift")
    routes = contract["routes"]
    require(len(routes) == 3, "Pass 5 requires three routes")
    require(all(set(item) == {"semantic_id", "can_id", "freshness_timeout_us"}
                for item in routes), "unexpected Pass 5 route fields")
    require(len({item["semantic_id"] for item in routes}) == 3, "duplicate receiver semantic route")
    require(len({item["can_id"] for item in routes}) == 3, "duplicate receiver CAN identifier")
    require(all(type(item["freshness_timeout_us"]) is int
                and item["freshness_timeout_us"] > 0 for item in routes),
            "invalid receiver freshness timeout")
    require({item["scenario_id"] for item in contract["queue_scenarios"]} == {
        "nominal_receiver", "constrained_receiver"
    }, "Pass 5 receiver scenario set drift")
    for scenario in contract["queue_scenarios"]:
        require(set(scenario) == {"scenario_id", "capacity", "service_time_us"},
                "unexpected receiver scenario fields")
        require(type(scenario["capacity"]) is int and scenario["capacity"] > 0,
                "invalid receiver queue capacity")
        require(type(scenario["service_time_us"]) is int and scenario["service_time_us"] > 0,
                "invalid receiver service time")
    vectors = contract["conformance_vectors"]
    require(len({item["case_id"] for item in vectors}) == len(vectors),
            "duplicate receiver conformance case")
    require(all(set(item) == {"case_id", "mutation", "expected"} for item in vectors),
            "unexpected receiver conformance-vector fields")
    require(contract["wire_capabilities"] == {
        "session_identity": "absent_from_payload_v1",
        "in_frame_codec_version": "absent_from_payload_v1",
        "sequence_reset_without_session": "reject_as_out_of_order",
    }, "Pass 5 wire-capability boundary drift")
    if not allow_unpinned:
        for key in ("expected_receiver_sha256", "expected_replay_sha256"):
            require(type(contract[key]) is str and len(contract[key]) == 64,
                    f"Pass 5 output is not pinned: {key}")


def validate_source(contract: dict[str, Any], transport_path: Path,
                    receipt_path: Path) -> dict[str, Any]:
    expected = contract["source_transport"]
    require(sha256_path(transport_path) == expected["sha256"],
            "Pass 4 transport hash mismatch")
    transport = load_json(transport_path)
    require(transport.get("schema") == expected["schema"], "Pass 4 transport schema mismatch")
    require(transport.get("project_id") == contract["project_id"],
            "Pass 4 transport project mismatch")
    require(len(transport.get("frames", [])) == expected["frames"],
            "Pass 4 frame count mismatch")
    scenarios = {item["scenario_id"]: item for item in transport["scenarios"]}
    require(expected["scenario_id"] in scenarios, "Pass 4 baseline scenario missing")
    require(len(scenarios[expected["scenario_id"]]["deliveries"]) == expected["deliveries"],
            "Pass 4 baseline delivery count mismatch")
    receipt = load_json(receipt_path)
    require(receipt.get("status") == "pass", "Pass 4 receipt is not passing")
    require(receipt.get("transport", {}).get("sha256") == expected["sha256"],
            "Pass 4 receipt join mismatch")
    return transport


def receive_frame(frame: dict[str, Any], contract: dict[str, Any], state: dict[str, Any],
                  *, codec: str | None = None, dlc: int | None = None,
                  can_id: int | None = None, payload_hex: str | None = None) -> dict[str, Any]:
    codec = contract["receiver"]["payload_codec"] if codec is None else codec
    if codec != contract["receiver"]["payload_codec"]:
        return {"disposition": "rejected_codec"}
    can_id = frame["can_id"] if can_id is None else can_id
    routes_by_id = {item["can_id"]: item for item in contract["routes"]}
    if can_id not in routes_by_id:
        return {"disposition": "rejected_unknown_can_id"}
    dlc = frame["dlc"] if dlc is None else dlc
    if dlc != 8:
        return {"disposition": "rejected_dlc"}
    payload_hex = frame["payload_hex"] if payload_hex is None else payload_hex
    try:
        payload = bytes.fromhex(payload_hex)
        decoded = can_transport.decode_payload(payload)
    except (ValueError, can_transport.TransportError):
        return {"disposition": "rejected_payload"}
    semantic_id = routes_by_id[can_id]["semantic_id"]
    previous = state.get(semantic_id)
    if previous is not None and decoded["sequence"] == previous:
        return {"disposition": "rejected_duplicate", "semantic_id": semantic_id,
                "sequence": decoded["sequence"]}
    if previous is not None and decoded["sequence"] < previous:
        return {"disposition": "rejected_out_of_order", "semantic_id": semantic_id,
                "sequence": decoded["sequence"]}
    state[semantic_id] = decoded["sequence"]
    disposition = ("accepted_latest_good" if decoded["valid"] and decoded["fresh"]
                   and decoded["usable"] else "accepted_degraded")
    return {"disposition": disposition, "semantic_id": semantic_id,
            "sequence": decoded["sequence"], "decoded_payload": decoded}


def baseline_arrivals(contract: dict[str, Any], transport: dict[str, Any]) -> list[dict[str, Any]]:
    frames = {item["frame_index"]: item for item in transport["frames"]}
    scenario = next(item for item in transport["scenarios"]
                    if item["scenario_id"] == contract["source_transport"]["scenario_id"])
    arrivals = []
    for delivery in scenario["deliveries"]:
        frame = frames[delivery["frame_index"]]
        require(frame["consumer_node"] == contract["receiver"]["node_id"],
                "Pass 4 delivery targets the wrong receiver")
        arrivals.append({"arrived_at_us": delivery["delivered_at_us"], "frame": frame})
    arrivals.sort(key=lambda item: (item["arrived_at_us"], item["frame"]["frame_index"]))
    return arrivals


def simulate_queue(contract: dict[str, Any], arrivals: list[dict[str, Any]],
                   scenario: dict[str, Any]) -> dict[str, Any]:
    queue: list[dict[str, Any]] = []
    cursor = 0
    service_ready_us = 0
    sequence_state: dict[str, int] = {}
    events: list[dict[str, Any]] = []
    latest_good: dict[str, dict[str, Any]] = {}
    high_water = 0

    while cursor < len(arrivals) or queue:
        next_arrival = arrivals[cursor]["arrived_at_us"] if cursor < len(arrivals) else None
        service_at = (max(service_ready_us, queue[0]["arrived_at_us"]) if queue else None)
        if next_arrival is not None and (service_at is None or next_arrival <= service_at):
            same_time = next_arrival
            while cursor < len(arrivals) and arrivals[cursor]["arrived_at_us"] == same_time:
                arrival = arrivals[cursor]
                frame = arrival["frame"]
                if len(queue) >= scenario["capacity"]:
                    events.append({
                        "event": "rx_drop", "at_us": same_time,
                        "frame_index": frame["frame_index"], "semantic_id": frame["semantic_id"],
                        "disposition": "dropped_rx_queue_overflow",
                    })
                else:
                    queue.append(arrival)
                    high_water = max(high_water, len(queue))
                    events.append({
                        "event": "rx_enqueue", "at_us": same_time,
                        "frame_index": frame["frame_index"], "semantic_id": frame["semantic_id"],
                        "queue_depth": len(queue),
                    })
                cursor += 1
            continue

        require(queue and service_at is not None, "receiver scheduler has no work")
        arrival = queue.pop(0)
        frame = arrival["frame"]
        decision = receive_frame(frame, contract, sequence_state)
        completed_at = service_at + scenario["service_time_us"]
        event = {
            "event": "rx_process", "at_us": completed_at,
            "arrived_at_us": arrival["arrived_at_us"],
            "receiver_queue_latency_us": service_at - arrival["arrived_at_us"],
            "frame_index": frame["frame_index"], "semantic_id": frame["semantic_id"],
            **decision,
        }
        events.append(event)
        if decision["disposition"] == "accepted_latest_good":
            latest_good[frame["semantic_id"]] = {
                "semantic_id": frame["semantic_id"], "sequence": decision["sequence"],
                "raw_value": decision["decoded_payload"]["raw_value"],
                "updated_at_us": completed_at, "frame_index": frame["frame_index"],
            }
        service_ready_us = completed_at

    processed = [item for item in events if item["event"] == "rx_process"]
    drops = [item for item in events if item["event"] == "rx_drop"]
    return {
        "scenario_id": scenario["scenario_id"],
        "configuration": {"capacity": scenario["capacity"],
                          "service_time_us": scenario["service_time_us"]},
        "events": events,
        "latest_good_before_expiry": [latest_good[key] for key in sorted(latest_good)],
        "statistics": {
            "arrivals": len(arrivals), "processed": len(processed),
            "rx_queue_overflow_drops": len(drops), "queue_high_water": high_water,
            "accepted_latest_good": sum(item.get("disposition") == "accepted_latest_good"
                                        for item in processed),
            "accepted_degraded": sum(item.get("disposition") == "accepted_degraded"
                                     for item in processed),
            "rejected_protocol": sum(item.get("disposition", "").startswith("rejected_")
                                     for item in processed),
            "maximum_receiver_queue_latency_us": max(
                (item["receiver_queue_latency_us"] for item in processed), default=0),
        },
    }


def freshness_projection(contract: dict[str, Any], nominal: dict[str, Any],
                         horizon_us: int) -> dict[str, Any]:
    routes = {item["semantic_id"]: item for item in contract["routes"]}
    updates: dict[str, list[dict[str, Any]]] = {key: [] for key in routes}
    for event in nominal["events"]:
        if event.get("disposition") == "accepted_latest_good":
            updates[event["semantic_id"]].append(event)
    transitions: list[dict[str, Any]] = []
    final_state = []
    for semantic_id in sorted(routes):
        timeout = routes[semantic_id]["freshness_timeout_us"]
        signal_updates = updates[semantic_id]
        require(bool(signal_updates), f"no latest-good update for {semantic_id}")
        deadline = signal_updates[0]["at_us"] + timeout
        stale = False
        for event in signal_updates[1:]:
            if event["at_us"] > deadline:
                transitions.append({"at_us": deadline, "semantic_id": semantic_id,
                                    "transition": "fresh_to_stale"})
                transitions.append({"at_us": event["at_us"], "semantic_id": semantic_id,
                                    "transition": "stale_to_fresh",
                                    "frame_index": event["frame_index"]})
                stale = False
            deadline = event["at_us"] + timeout
        if deadline <= horizon_us:
            transitions.append({"at_us": deadline, "semantic_id": semantic_id,
                                "transition": "fresh_to_stale"})
            stale = True
        last = signal_updates[-1]
        final_state.append({
            "semantic_id": semantic_id, "state_at_horizon": "stale" if stale else "fresh",
            "horizon_us": horizon_us, "latest_good_sequence": last["sequence"],
            "latest_good_raw_value": last["decoded_payload"]["raw_value"],
            "latest_good_updated_at_us": last["at_us"], "freshness_deadline_us": deadline,
        })
    transitions.sort(key=lambda item: (item["at_us"], item["semantic_id"], item["transition"]))
    return {"horizon_us": horizon_us, "transitions": transitions, "final_state": final_state}


def rewrite_payload(frame: dict[str, Any], *, sequence: int | None = None,
                    status_mask_set: int = 0, status_mask_clear: int = 0) -> dict[str, Any]:
    candidate = copy.deepcopy(frame)
    payload = bytearray.fromhex(candidate["payload_hex"])
    if sequence is not None:
        payload[0:2] = sequence.to_bytes(2, "little")
    payload[6] = (payload[6] | status_mask_set) & (~status_mask_clear & 0xFF)
    candidate["payload_hex"] = bytes(payload).hex()
    metrics = can_transport.frame_metrics(candidate["can_id"], bytes(payload))
    candidate.update(metrics)
    return candidate


def conformance_projection(contract: dict[str, Any], transport: dict[str, Any]) -> list[dict[str, Any]]:
    base = next(frame for frame in transport["frames"]
                if frame["semantic_id"] == "signal.bench.ultrasonic.distance"
                and frame["decoded_payload"]["sequence"] == 2)
    results = []
    for vector in contract["conformance_vectors"]:
        mutation = vector["mutation"]
        state: dict[str, int] = {}
        frame = copy.deepcopy(base)
        kwargs: dict[str, Any] = {}
        if mutation == "duplicate":
            receive_frame(frame, contract, state)
        elif mutation == "out_of_order":
            receive_frame(rewrite_payload(frame, sequence=3), contract, state)
        elif mutation == "sequence_reset":
            receive_frame(rewrite_payload(frame, sequence=10), contract, state)
            frame = rewrite_payload(frame, sequence=0)
        elif mutation == "unknown_can_id":
            kwargs["can_id"] = 0x7FF
        elif mutation == "wrong_dlc":
            kwargs["dlc"] = 7
        elif mutation == "reserved_status":
            frame = rewrite_payload(frame, status_mask_set=0x80)
        elif mutation == "wrong_codec":
            kwargs["codec"] = "fisiCs.sensor_scalar_le_v2"
        elif mutation == "invalid":
            frame = rewrite_payload(frame, status_mask_clear=0x01)
        elif mutation == "stale":
            frame = rewrite_payload(frame, status_mask_clear=0x02)
        elif mutation == "unusable":
            frame = rewrite_payload(frame, status_mask_clear=0x04)
        elif mutation != "none":
            raise ReceiverError(f"unknown conformance mutation: {mutation}")
        decision = receive_frame(frame, contract, state, **kwargs)
        require(decision["disposition"] == vector["expected"],
                f"conformance case {vector['case_id']} expected {vector['expected']} "
                f"but got {decision['disposition']}")
        results.append({**vector, "actual": decision["disposition"], "passed": True})
    return results


def emit_receiver(contract: dict[str, Any], transport: dict[str, Any]) -> dict[str, Any]:
    arrivals = baseline_arrivals(contract, transport)
    queue_results = [simulate_queue(contract, arrivals, scenario)
                     for scenario in contract["queue_scenarios"]]
    nominal = next(item for item in queue_results if item["scenario_id"] == "nominal_receiver")
    source_horizon = next(item for item in transport["scenarios"]
                          if item["scenario_id"] == contract["source_transport"]["scenario_id"])[
                              "statistics"]["simulation_horizon_us"]
    horizon = source_horizon + max(item["freshness_timeout_us"] for item in contract["routes"]) + 500000
    return {
        "schema": "fisiCs.esp_can_receiver_projection", "schema_version": 1,
        "project_id": contract["project_id"], "source_transport": contract["source_transport"],
        "receiver": contract["receiver"], "routes": contract["routes"],
        "wire_capabilities": contract["wire_capabilities"],
        "queue_scenarios": queue_results,
        "freshness": freshness_projection(contract, nominal, horizon),
        "conformance": conformance_projection(contract, transport),
    }


def emit_replay(receiver: dict[str, Any], transport: dict[str, Any]) -> dict[str, Any]:
    nominal = next(item for item in receiver["queue_scenarios"]
                   if item["scenario_id"] == "nominal_receiver")
    baseline = next(item for item in transport["scenarios"]
                    if item["scenario_id"] == "baseline_transient_ack_loss")
    return {
        "schema": "fisiCs.esp_can_debug_replay_slices", "schema_version": 1,
        "source_transport_sha256": receiver["source_transport"]["sha256"],
        "slices": [
            {
                "slice_id": "first_second_arbitration_and_retry",
                "start_us": 1000000, "end_us": 1005000,
                "transport_attempts": [item for item in baseline["attempts"]
                                       if 1000000 <= item["start_at_us"] <= 1005000],
                "receiver_events": [item for item in nominal["events"]
                                    if 1000000 <= item["at_us"] <= 1005000],
            },
            {
                "slice_id": "freshness_transitions",
                "events": receiver["freshness"]["transitions"],
                "final_state": receiver["freshness"]["final_state"],
            },
            {
                "slice_id": "protocol_conformance",
                "cases": receiver["conformance"],
                "wire_capabilities": receiver["wire_capabilities"],
            },
        ],
    }


def validate_receiver(contract: dict[str, Any], receiver: dict[str, Any],
                      transport: dict[str, Any]) -> None:
    require(receiver.get("source_transport") == contract["source_transport"],
            "receiver source transport mismatch")
    require(receiver.get("wire_capabilities") == contract["wire_capabilities"],
            "receiver wire-capability boundary mismatch")
    expected = emit_receiver(contract, transport)
    require(canonical_bytes(receiver) == canonical_bytes(expected),
            "receiver deterministic replay mismatch")
    scenarios = {item["scenario_id"]: item for item in receiver["queue_scenarios"]}
    nominal = scenarios["nominal_receiver"]["statistics"]
    require(nominal["arrivals"] == 149 and nominal["processed"] == 149,
            "nominal receiver did not process all frames")
    require(nominal["rx_queue_overflow_drops"] == 0, "nominal receiver overflowed")
    require(nominal["accepted_latest_good"] == 136,
            "nominal latest-good acceptance count drift")
    require(nominal["accepted_degraded"] == 3, "nominal degraded count drift")
    require(nominal["rejected_protocol"] == 10,
            "nominal duplicate/out-of-order rejection count drift")
    constrained = scenarios["constrained_receiver"]["statistics"]
    require(constrained["rx_queue_overflow_drops"] > 0,
            "constrained receiver did not exercise backpressure")
    require(all(item["passed"] for item in receiver["conformance"]),
            "receiver conformance vector failed")
    require(any(item["transition"] == "fresh_to_stale"
                and item["semantic_id"] == "signal.bench.ultrasonic.distance"
                for item in receiver["freshness"]["transitions"]),
            "distance freshness expiry was not observed")


def validate_replay(receiver: dict[str, Any], replay: dict[str, Any],
                    transport: dict[str, Any]) -> None:
    require(canonical_bytes(replay) == canonical_bytes(emit_replay(receiver, transport)),
            "debug replay slice mismatch")
    require({item["slice_id"] for item in replay["slices"]} == {
        "first_second_arbitration_and_retry", "freshness_transitions", "protocol_conformance"
    }, "debug replay slice coverage mismatch")


def expect_failure(contract: dict[str, Any], receiver: dict[str, Any], transport: dict[str, Any],
                   mutate: Callable[[dict[str, Any]], None], label: str) -> str:
    candidate = copy.deepcopy(receiver)
    mutate(candidate)
    try:
        validate_receiver(contract, candidate, transport)
    except (ReceiverError, KeyError, ValueError):
        return label
    raise ReceiverError(f"negative receiver unexpectedly passed: {label}")


def negative_proof(contract: dict[str, Any], receiver: dict[str, Any],
                   transport: dict[str, Any]) -> list[str]:
    failures = [
        expect_failure(contract, receiver, transport,
                       lambda value: value["source_transport"].__setitem__("sha256", "0" * 64),
                       "wrong_transport_hash_rejected"),
        expect_failure(contract, receiver, transport,
                       lambda value: value["queue_scenarios"][0]["statistics"].__setitem__(
                           "accepted_latest_good", 0), "latest_good_count_mismatch_rejected"),
        expect_failure(contract, receiver, transport,
                       lambda value: value["freshness"]["transitions"].clear(),
                       "missing_freshness_transitions_rejected"),
        expect_failure(contract, receiver, transport,
                       lambda value: value["conformance"][0].__setitem__("actual", "accepted_degraded"),
                       "conformance_disposition_mismatch_rejected"),
        expect_failure(contract, receiver, transport,
                       lambda value: value["wire_capabilities"].__setitem__(
                           "session_identity", "inferred"), "invented_session_identity_rejected"),
    ]
    bad_contract = copy.deepcopy(contract)
    bad_contract["queue_scenarios"][0]["capacity"] = 0
    try:
        validate_contract(bad_contract)
    except ReceiverError:
        failures.append("zero_receiver_capacity_rejected")
    else:
        raise ReceiverError("negative receiver contract unexpectedly passed")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture-root", type=Path, default=DEFAULT_FIXTURE_ROOT)
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    parser.add_argument("--transport", type=Path, default=DEFAULT_TRANSPORT)
    parser.add_argument("--pass4-receipt", type=Path, default=DEFAULT_PASS4_RECEIPT)
    parser.add_argument("--allow-unpinned", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()

    fixture_root = args.fixture_root.resolve()
    build_root = args.build_root.resolve()
    transport_path = args.transport.resolve()
    receipt_path = args.pass4_receipt.resolve()
    require(fixture_root == DEFAULT_FIXTURE_ROOT.resolve(), "runner accepts only fixed Pass 5 fixture")
    require(build_root == DEFAULT_BUILD_ROOT.resolve(), "runner accepts only fixed Pass 5 build root")
    require(transport_path == DEFAULT_TRANSPORT.resolve(), "runner accepts only fixed Pass 4 transport")
    require(receipt_path == DEFAULT_PASS4_RECEIPT.resolve(), "runner accepts only fixed Pass 4 receipt")

    contract_path = fixture_root / "contract.json"
    contract = load_json(contract_path)
    validate_contract(contract, allow_unpinned=args.allow_unpinned)
    transport = validate_source(contract, transport_path, receipt_path)
    receiver = emit_receiver(contract, transport)
    validate_receiver(contract, receiver, transport)
    replay = emit_replay(receiver, transport)
    validate_replay(receiver, replay, transport)
    build_root.mkdir(parents=True, exist_ok=True)
    receiver_path = build_root / "receiver.json"
    replay_path = build_root / "replay-slices.json"
    canonical_write(receiver_path, receiver)
    canonical_write(replay_path, replay)
    receiver_hash = sha256_path(receiver_path)
    replay_hash = sha256_path(replay_path)
    if not args.allow_unpinned:
        require(receiver_hash == contract["expected_receiver_sha256"], "Pass 5 receiver hash drift")
        require(replay_hash == contract["expected_replay_sha256"], "Pass 5 replay hash drift")
    negatives = negative_proof(contract, receiver, transport)
    scenarios = {item["scenario_id"]: item["statistics"] for item in receiver["queue_scenarios"]}
    receipt = {
        "schema": "fisiCs.esp_can_receiver_receipt", "schema_version": 1,
        "project_id": contract["project_id"], "status": "pass",
        "contract": {"path": str(contract_path), "sha256": sha256_path(contract_path)},
        "source_transport": {"path": str(transport_path), "sha256": sha256_path(transport_path),
                             "pass4_receipt_path": str(receipt_path),
                             "pass4_receipt_sha256": sha256_path(receipt_path)},
        "receiver": {"path": str(receiver_path), "sha256": receiver_hash,
                     "byte_deterministic": True},
        "replay_slices": {"path": str(replay_path), "sha256": replay_hash,
                          "byte_deterministic": True, "count": len(replay["slices"])},
        "queue_scenarios": scenarios,
        "freshness": {"transitions": len(receiver["freshness"]["transitions"]),
                      "final_state": receiver["freshness"]["final_state"]},
        "conformance": {"vectors": len(receiver["conformance"]), "passed": True,
                        "session_restart_disambiguation": "not_representable_in_payload_v1"},
        "negative_proof": negatives,
        "claim_boundary": {
            "receiver_queue": "deterministically_simulated",
            "payload_admission": "deterministically_simulated",
            "latest_good_state": "deterministically_simulated",
            "freshness_expiry": "deterministically_simulated",
            "duplicate_and_out_of_order": "conformance_simulated",
            "session_restart": "not_representable_in_payload_v1",
            "in_frame_version_negotiation": "not_representable_in_payload_v1",
            "debug_replay": "generated_static_slices",
            "native_visualizer_consumption": "not_assessed",
            "esp_twai_driver": "not_assessed",
            "runtime_hardware_observation": "not_assessed",
            "physical_acceptance": "not_assessed"
        },
    }
    canonical_write(build_root / "receipt.json", receipt)
    nominal = scenarios["nominal_receiver"]
    constrained = scenarios["constrained_receiver"]
    print("ESP CAN Pass 5=PASS "
          f"processed={nominal['processed']} latest_good={nominal['accepted_latest_good']} "
          f"degraded={nominal['accepted_degraded']} rx_overflow={constrained['rx_queue_overflow_drops']} "
          f"freshness_transitions={len(receiver['freshness']['transitions'])} "
          f"vectors={len(receiver['conformance'])} negatives={len(negatives)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ReceiverError as exc:
        print(f"ESP CAN Pass 5: FAIL: {exc}", file=os.sys.stderr)
        raise SystemExit(1)
