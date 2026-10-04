#!/usr/bin/env python3
"""Pass 6 actual ESP protocol-v1 framing, heartbeat, and session-state proof."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import subprocess
from typing import Any, Callable


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_FIXTURE_ROOT = FISICS_ROOT / "tests/fixtures/esp_can_protocol_session_v1"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-can-protocol-session"
PASS3_OBSERVATION = FISICS_ROOT / "build/integration-esp-can-simulation/simulated-observation.json"
PASS5_RECEIVER = FISICS_ROOT / "build/integration-esp-can-receiver/receiver.json"
PASS5_RECEIPT = FISICS_ROOT / "build/integration-esp-can-receiver/receipt.json"
CRC15_POLYNOMIAL = 0x4599


class ProtocolSessionError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ProtocolSessionError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ProtocolSessionError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root must be an object: {path}")
    return value


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def canonical_write(path: Path, value: Any) -> None:
    path.write_bytes(canonical_bytes(value))


def sha256_path(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def git(root: Path, *arguments: str) -> str:
    result = subprocess.run(["git", "-C", str(root), *arguments], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise ProtocolSessionError(f"git {' '.join(arguments)} failed:\n{result.stdout}")
    return result.stdout


def validate_contract(contract: dict[str, Any], *, allow_unpinned: bool = False) -> None:
    require(set(contract) == {"schema", "schema_version", "project_id", "source_inputs",
                              "esp_protocol", "node_bindings", "signal_bindings",
                              "runtime_policy", "expected_projection_sha256",
                              "expected_replay_sha256"}, "unexpected Pass 6 contract fields")
    require(contract["schema"] == "fisiCs.esp_can_protocol_session_contract",
            "unsupported Pass 6 schema")
    require(contract["schema_version"] == 1, "unsupported Pass 6 schema version")
    require(contract["project_id"] == "esp.can_pass2.sensor_backbone", "Pass 6 project drift")
    require(contract["source_inputs"] == {
        "pass3_observation_sha256": "e142643a7a361431ceac65e38a9b595be58c81bc33e26ad74e5687b922581a35",
        "pass5_receiver_sha256": "caf657b7c62ceba44da209feef08d1b1ac1a71289da709e0678c23c19ff56491",
    }, "Pass 6 source input drift")
    protocol = contract["esp_protocol"]
    require(protocol["schema_identity_sha256"] ==
            "2970b24a30d90795ac5f14c2953cc7f1cbe0c437873ed23ce241f115755e921f",
            "Pass 6 protocol identity drift")
    require(protocol["protocol_version"] == 1 and protocol["registry_version"] == 2,
            "Pass 6 protocol version drift")
    require(protocol["wire"] == {"can": "classical", "identifier": "extended_29_bit",
                                  "byte_order": "big_endian", "dlc": 8},
            "Pass 6 wire contract drift")
    require({item["protocol_source"] for item in contract["node_bindings"]} == {"BENCH_A", "BENCH_C"},
            "Pass 6 protocol source set drift")
    require(len({item["session_id"] for item in contract["node_bindings"]}) == 2,
            "Pass 6 session IDs must be distinct")
    require({item["semantic_type"] for item in contract["signal_bindings"]} ==
            {"DISTANCE", "AIR_TEMPERATURE", "RELATIVE_HUMIDITY"},
            "Pass 6 signal type set drift")
    require(len({item["semantic_id"] for item in contract["signal_bindings"]}) == 3,
            "duplicate Pass 6 semantic binding")
    require(contract["runtime_policy"] == {
        "bitrate": 500000, "publication_quantum_us": 1000000,
        "heartbeat_period_us": 1000000, "heartbeat_timeout_us": 2500000,
        "startup_signal_delay_us": 1000,
        "startup_requirement": "initial_heartbeat_must_complete_before_signal_release",
        "session_change": "clear_source_sequences_and_latest_good",
        "offline_signal": "reject", "same_session_recovery": "retain_sequence_state",
        "producer_restart_requirement": "flush_pending_signal_frames_before_new_session_heartbeat",
    }, "Pass 6 runtime policy drift")
    if not allow_unpinned:
        for key in ("expected_projection_sha256", "expected_replay_sha256"):
            require(type(contract[key]) is str and len(contract[key]) == 64,
                    f"Pass 6 output is not pinned: {key}")


def verify_external_inputs(esp_root: Path, contract: dict[str, Any]) -> list[dict[str, str]]:
    protocol = contract["esp_protocol"]
    results = []
    for name in ("schema", "generated_header", "golden_vectors"):
        relative = protocol[f"{name}_path"]
        path = (esp_root / relative).resolve()
        require(path.is_relative_to(esp_root) and path.is_file(), f"missing ESP protocol input: {relative}")
        actual = sha256_path(path)
        require(actual == protocol[f"{name}_sha256"], f"ESP protocol input drift: {name}")
        results.append({"role": name, "path": relative, "sha256": actual})
    schema = load_json(esp_root / protocol["schema_path"])
    require(schema["protocol_version"] == protocol["protocol_version"], "ESP protocol version mismatch")
    require(schema["registry_version"] == protocol["registry_version"], "ESP registry version mismatch")
    require(schema["wire"] == {
        "can": "classical", "identifier": "extended_29_bit", "frame": "data_only",
        "byte_order": "big_endian", "max_dlc": 8,
        "id_layout": {"priority": {"shift": 26, "bits": 3},
                      "family": {"shift": 22, "bits": 4},
                      "source": {"shift": 16, "bits": 6},
                      "subject": {"shift": 0, "bits": 16}},
    }, "ESP protocol wire schema mismatch")
    golden = load_json(esp_root / protocol["golden_vectors_path"])
    require(golden["schema_sha256"] == protocol["schema_identity_sha256"],
            "ESP golden vectors use the wrong schema identity")
    return results


def run_external_codec_gate(esp_root: Path) -> dict[str, Any]:
    result = subprocess.run(["make", "-C", str(esp_root), "test-protocol"], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        raise ProtocolSessionError(f"ESP protocol host gate failed:\n{result.stdout}")
    require("protocol v1 C++ tests: PASS" in result.stdout,
            "ESP protocol C++ gate did not report success")
    require("Ran 16 tests" in result.stdout and "OK" in result.stdout,
            "ESP protocol Python gate did not report the expected suite")
    return {"command": "make test-protocol", "status": "pass",
            "cpp": "pass", "python_tests": 16,
            "output_sha256": hashlib.sha256(result.stdout.encode()).hexdigest()}


def validate_prerequisites(contract: dict[str, Any]) -> tuple[dict[str, Any], dict[str, Any]]:
    require(sha256_path(PASS3_OBSERVATION) == contract["source_inputs"]["pass3_observation_sha256"],
            "Pass 3 observation hash mismatch")
    require(sha256_path(PASS5_RECEIVER) == contract["source_inputs"]["pass5_receiver_sha256"],
            "Pass 5 receiver hash mismatch")
    receipt = load_json(PASS5_RECEIPT)
    require(receipt.get("status") == "pass", "Pass 5 receipt is not passing")
    require(receipt.get("receiver", {}).get("sha256") ==
            contract["source_inputs"]["pass5_receiver_sha256"], "Pass 5 receipt join mismatch")
    return load_json(PASS3_OBSERVATION), load_json(PASS5_RECEIVER)


def bits(value: int, width: int) -> list[int]:
    require(0 <= value < (1 << width), f"value {value} exceeds {width} bits")
    return [(value >> shift) & 1 for shift in range(width - 1, -1, -1)]


def crc15(values: list[int]) -> int:
    remainder = 0
    for bit in values:
        feedback = ((remainder >> 14) & 1) ^ bit
        remainder = (remainder << 1) & 0x7FFF
        if feedback:
            remainder ^= CRC15_POLYNOMIAL
    return remainder


def stuff(values: list[int]) -> int:
    previous = values[0]
    run = 1
    inserted = 0
    for bit in values[1:]:
        if bit == previous:
            run += 1
        else:
            previous = bit
            run = 1
        if run == 5:
            previous = 1 - bit
            run = 1
            inserted += 1
    return inserted


def extended_frame_metrics(can_id: int, payload: bytes) -> dict[str, Any]:
    require(0 <= can_id <= 0x1FFFFFFF, "extended CAN identifier out of range")
    require(len(payload) == 8, "protocol v1 requires DLC 8")
    base = (([0] + bits(can_id >> 18, 11) + [1, 1] + bits(can_id & 0x3FFFF, 18)
             + [0, 0, 0] + bits(8, 4)
             + [bit for byte in payload for bit in bits(byte, 8)]))
    checksum = crc15(base)
    stuffed = stuff(base + bits(checksum, 15))
    return {"crc15": f"0x{checksum:04x}", "stuff_bits": stuffed,
            "unstuffed_bits_including_intermission": 131,
            "total_wire_bits": 131 + stuffed}


def pack_identifier(priority: int, family: int, source: int, subject: int) -> int:
    require(0 <= priority < 8 and 0 <= family < 16 and 0 <= source < 64
            and 0 <= subject < 65536, "invalid protocol identifier field")
    return (priority << 26) | (family << 22) | (source << 16) | subject


def encode_heartbeat(source: int, session: int, *, version: int = 1,
                     health: int = 0, faults: int = 0, reserved: int = 0) -> dict[str, Any]:
    payload = bytes((version,)) + session.to_bytes(4, "big") + bytes((health, faults, reserved))
    can_id = pack_identifier(5, 1, source, 0)
    return {"kind": "heartbeat", "can_id": can_id, "can_id_hex": f"0x{can_id:08X}",
            "extended": True, "remote": False, "dlc": 8, "data_hex": payload.hex().upper(),
            **extended_frame_metrics(can_id, payload)}


def binding_maps(contract: dict[str, Any]) -> tuple[dict[str, Any], dict[str, Any]]:
    nodes = {item["protocol_source"]: item for item in contract["node_bindings"]}
    signals = {item["semantic_id"]: item for item in contract["signal_bindings"]}
    return nodes, signals


def encode_signal(record: dict[str, Any], contract: dict[str, Any]) -> dict[str, Any]:
    nodes, signals = binding_maps(contract)
    binding = signals[record["semantic_id"]]
    node = nodes[binding["protocol_source"]]
    raw = record["raw_value"]
    sequence = record["sequence"]
    require(0 <= sequence <= 255, "protocol v1 signal sequence exceeds u8")
    width = 2
    signed = binding["encoding"].startswith("i")
    minimum, maximum = (-(1 << 15), (1 << 15) - 1) if signed else (0, (1 << 16) - 1)
    require(minimum <= raw <= maximum, "protocol v1 signal value out of range")
    # Protocol v1 has one source-validity field, not separate quality/fresh/usable bits.
    validity = 0 if record["validity"] == 0 and record["fresh"] and record["usable"] else 1
    encoded = raw.to_bytes(width, "big", signed=signed)
    payload = bytes((binding["instance_id"], sequence, validity, 0)) + encoded + bytes(2)
    can_id = pack_identifier(4, 2, node["source_id"], binding["type_id"])
    return {"kind": "signal", "semantic_id": record["semantic_id"],
            "protocol_source": binding["protocol_source"], "source_id": node["source_id"],
            "semantic_type": binding["semantic_type"], "type_id": binding["type_id"],
            "instance": binding["instance"], "instance_id": binding["instance_id"],
            "sequence": sequence, "source_validity": validity, "raw_value": raw,
            "can_id": can_id, "can_id_hex": f"0x{can_id:08X}", "extended": True,
            "remote": False, "dlc": 8, "data_hex": payload.hex().upper(),
            **extended_frame_metrics(can_id, payload)}


def decode_identifier(can_id: int) -> dict[str, int]:
    return {"priority": (can_id >> 26) & 0x7, "family": (can_id >> 22) & 0xF,
            "source_id": (can_id >> 16) & 0x3F, "subject": can_id & 0xFFFF}


def decode_frame(frame: dict[str, Any], contract: dict[str, Any]) -> tuple[str, dict[str, Any]]:
    if frame.get("extended") is not True or frame.get("remote") is not False:
        return "rejected_frame_kind", {}
    if frame.get("dlc") != 8:
        return "rejected_dlc", {}
    try:
        payload = bytes.fromhex(frame["data_hex"])
    except (ValueError, KeyError):
        return "rejected_payload", {}
    if len(payload) != 8:
        return "rejected_payload", {}
    fields = decode_identifier(frame["can_id"])
    if fields["source_id"] not in {item["source_id"] for item in contract["node_bindings"]}:
        return "rejected_source", {}
    if fields["priority"] == 5 and fields["family"] == 1 and fields["subject"] == 0:
        if payload[0] != contract["esp_protocol"]["protocol_version"]:
            return "rejected_protocol_version", {}
        if payload[7] != 0:
            return "rejected_reserved", {}
        return "heartbeat", {"source_id": fields["source_id"],
                             "session_id": int.from_bytes(payload[1:5], "big"),
                             "health_flags": payload[5], "active_fault_count": payload[6]}
    bindings = {item["type_id"]: item for item in contract["signal_bindings"]}
    if fields["priority"] != 4 or fields["family"] != 2 or fields["subject"] not in bindings:
        return "rejected_identifier", {}
    binding = bindings[fields["subject"]]
    expected_node = next(item for item in contract["node_bindings"]
                         if item["protocol_source"] == binding["protocol_source"])
    if fields["source_id"] != expected_node["source_id"]:
        return "rejected_source_binding", {}
    if payload[0] != binding["instance_id"]:
        return "rejected_instance", {}
    if payload[2] not in (0, 1, 2):
        return "rejected_validity", {}
    if payload[3] != 0 or payload[6:] != bytes(2):
        return "rejected_reserved", {}
    signed = binding["encoding"].startswith("i")
    return "signal", {"source_id": fields["source_id"], "semantic_id": binding["semantic_id"],
                      "sequence": payload[1], "source_validity": payload[2],
                      "raw_value": int.from_bytes(payload[4:6], "big", signed=signed)}


def verify_golden_vectors(esp_root: Path, contract: dict[str, Any]) -> list[dict[str, Any]]:
    golden = load_json(esp_root / contract["esp_protocol"]["golden_vectors_path"])
    vectors = {item["name"]: item for item in golden["vectors"]}
    checks = []
    heartbeat = encode_heartbeat(1, 0x12345678)
    for key in ("can_id_hex", "data_hex", "dlc", "extended", "remote"):
        require(heartbeat[key] == vectors["bench_a_heartbeat"][key],
                f"heartbeat golden mismatch: {key}")
    checks.append({"name": "bench_a_heartbeat", "matched": True})
    examples = [
        ("ultrasonic_106mm", "signal.bench.ultrasonic.distance", 4, 106),
        ("rear_left_temperature", "signal.bench.environment.air_temperature", 9, 2345),
    ]
    for name, semantic_id, sequence, raw in examples:
        record = {"semantic_id": semantic_id, "sequence": sequence, "raw_value": raw,
                  "validity": 0, "fresh": True, "usable": True}
        frame = encode_signal(record, contract)
        for key in ("can_id_hex", "data_hex", "dlc", "extended", "remote"):
            require(frame[key] == vectors[name][key], f"{name} golden mismatch: {key}")
        checks.append({"name": name, "matched": True})
    return checks


def ceil_quantum(value: int, quantum: int) -> int:
    return ((value + quantum - 1) // quantum) * quantum


def emit_wire_projection(contract: dict[str, Any], observation: dict[str, Any]) -> dict[str, Any]:
    policy = contract["runtime_policy"]
    nodes, _ = binding_maps(contract)
    pending = []
    for source_index, record in enumerate(observation["records"]):
        frame = encode_signal(record, contract)
        release_at = ceil_quantum(record["at_us"], policy["publication_quantum_us"])
        if release_at == 0:
            release_at = policy["startup_signal_delay_us"]
        pending.append({"release_at_us": release_at,
                        "source_at_us": record["at_us"], "source_record_index": source_index, **frame})
    maximum_release = max(item["release_at_us"] for item in pending)
    for at_us in range(0, maximum_release + 1, policy["heartbeat_period_us"]):
        for source_name in sorted(nodes):
            node = nodes[source_name]
            pending.append({"release_at_us": at_us, "source_at_us": at_us,
                            "protocol_source": source_name, "source_id": node["source_id"],
                            "session_id": node["session_id"],
                            **encode_heartbeat(node["source_id"], node["session_id"])})
    pending.sort(key=lambda item: (item["release_at_us"], item["can_id"], item["kind"],
                                  item.get("source_record_index", -1), item["source_id"]))
    now_us = 0
    frames = []
    for index, item in enumerate(pending):
        start = max(now_us, item["release_at_us"])
        duration = (item["total_wire_bits"] * 1_000_000 + policy["bitrate"] - 1) // policy["bitrate"]
        end = start + duration
        frames.append({"wire_index": index, **item, "start_at_us": start,
                       "end_at_us": end, "transport_latency_us": end - item["release_at_us"]})
        now_us = end
    return {"schema": "fisiCs.esp_protocol_v1_wire_projection", "schema_version": 1,
            "project_id": contract["project_id"],
            "source_observation_sha256": contract["source_inputs"]["pass3_observation_sha256"],
            "protocol": {"schema_identity_sha256": contract["esp_protocol"]["schema_identity_sha256"],
                         "protocol_version": 1, "identifier": "extended_29_bit",
                         "byte_order": "big_endian"},
            "runtime_policy": policy, "frames": frames,
            "statistics": {"signal_frames": len(observation["records"]),
                           "heartbeat_frames": len(frames) - len(observation["records"]),
                           "total_frames": len(frames),
                           "wire_bits": sum(item["total_wire_bits"] for item in frames),
                           "maximum_transport_latency_us": max(item["transport_latency_us"] for item in frames),
                           "horizon_us": now_us}}


def new_receiver_state() -> dict[str, Any]:
    return {"sources": {}, "sequences": {}, "latest_good": {}}


def expire_sources(state: dict[str, Any], at_us: int, timeout_us: int,
                   transitions: list[dict[str, Any]]) -> None:
    for source_id, source in state["sources"].items():
        if source["online"] and at_us > source["last_heartbeat_us"] + timeout_us:
            source["online"] = False
            transitions.append({"at_us": source["last_heartbeat_us"] + timeout_us,
                                "source_id": int(source_id), "transition": "online_to_offline"})


def process_frame(frame: dict[str, Any], contract: dict[str, Any], state: dict[str, Any],
                  at_us: int, transitions: list[dict[str, Any]]) -> dict[str, Any]:
    expire_sources(state, at_us, contract["runtime_policy"]["heartbeat_timeout_us"], transitions)
    kind, decoded = decode_frame(frame, contract)
    if kind.startswith("rejected_"):
        return {"disposition": kind}
    source_key = str(decoded["source_id"])
    if kind == "heartbeat":
        prior = state["sources"].get(source_key)
        if prior is None:
            disposition = "accepted_session_start"
        elif prior["session_id"] != decoded["session_id"]:
            disposition = "accepted_session_restart"
            owned = [item["semantic_id"] for item in contract["signal_bindings"]
                     if next(node["source_id"] for node in contract["node_bindings"]
                             if node["protocol_source"] == item["protocol_source"]) == decoded["source_id"]]
            for semantic_id in owned:
                state["sequences"].pop(semantic_id, None)
                state["latest_good"].pop(semantic_id, None)
        elif not prior["online"]:
            disposition = "accepted_session_recovery"
        else:
            disposition = "accepted_heartbeat_refresh"
        state["sources"][source_key] = {"session_id": decoded["session_id"],
                                        "last_heartbeat_us": at_us, "online": True,
                                        "health_flags": decoded["health_flags"],
                                        "active_fault_count": decoded["active_fault_count"]}
        return {"disposition": disposition, **decoded}
    source = state["sources"].get(source_key)
    if source is None or not source["online"]:
        return {"disposition": "rejected_no_live_session", **decoded}
    semantic_id = decoded["semantic_id"]
    previous = state["sequences"].get(semantic_id)
    if previous is not None and decoded["sequence"] == previous:
        return {"disposition": "rejected_duplicate", **decoded}
    if previous is not None and decoded["sequence"] < previous:
        return {"disposition": "rejected_out_of_order", **decoded}
    state["sequences"][semantic_id] = decoded["sequence"]
    if decoded["source_validity"] == 0:
        state["latest_good"][semantic_id] = {"sequence": decoded["sequence"],
                                             "raw_value": decoded["raw_value"],
                                             "session_id": source["session_id"], "updated_at_us": at_us}
        disposition = "accepted_latest_good"
    else:
        disposition = "accepted_degraded"
    return {"disposition": disposition, "session_id": source["session_id"], **decoded}


def run_nominal_receiver(contract: dict[str, Any], projection: dict[str, Any]) -> dict[str, Any]:
    state = new_receiver_state()
    transitions: list[dict[str, Any]] = []
    events = []
    for frame in projection["frames"]:
        decision = process_frame(frame, contract, state, frame["end_at_us"], transitions)
        events.append({"wire_index": frame["wire_index"], "at_us": frame["end_at_us"],
                       "kind": frame["kind"], "protocol_source": frame["protocol_source"], **decision})
    final_at = projection["statistics"]["horizon_us"] + contract["runtime_policy"]["heartbeat_timeout_us"] + 1
    expire_sources(state, final_at, contract["runtime_policy"]["heartbeat_timeout_us"], transitions)
    return {"events": events, "transitions": transitions, "final_at_us": final_at,
            "final_state": state,
            "statistics": {"events": len(events),
                           "session_starts": sum(item["disposition"] == "accepted_session_start" for item in events),
                           "heartbeat_refreshes": sum(item["disposition"] == "accepted_heartbeat_refresh" for item in events),
                           "latest_good_updates": sum(item["disposition"] == "accepted_latest_good" for item in events),
                           "degraded_updates": sum(item["disposition"] == "accepted_degraded" for item in events),
                           "duplicate_rejections": sum(item["disposition"] == "rejected_duplicate" for item in events),
                           "no_session_rejections": sum(item["disposition"] == "rejected_no_live_session" for item in events),
                           "offline_transitions": sum(item["transition"] == "online_to_offline" for item in transitions)}}


def conformance_cases(contract: dict[str, Any]) -> list[dict[str, Any]]:
    distance = {"semantic_id": "signal.bench.ultrasonic.distance", "sequence": 200,
                "raw_value": 106, "validity": 0, "fresh": True, "usable": True}
    distance0 = {**distance, "sequence": 0}
    signal200 = encode_signal(distance, contract)
    signal0 = encode_signal(distance0, contract)
    hb1 = encode_heartbeat(1, 0xA0010001)
    hb2 = encode_heartbeat(1, 0xA0010002)
    cases = []

    def run_case(name: str, timeline: list[tuple[int, dict[str, Any]]], expected: list[str],
                 note: str | None = None) -> None:
        state = new_receiver_state()
        transitions: list[dict[str, Any]] = []
        actual = [process_frame(frame, contract, state, at_us, transitions)["disposition"]
                  for at_us, frame in timeline]
        require(actual == expected, f"Pass 6 conformance case failed: {name}: {actual}")
        item = {"case_id": name, "expected": expected, "actual": actual, "passed": True}
        if note:
            item["boundary_note"] = note
        cases.append(item)

    run_case("session_start_and_signal", [(0, hb1), (100, signal200)],
             ["accepted_session_start", "accepted_latest_good"])
    run_case("session_restart_accepts_sequence_reset", [(0, hb1), (100, signal200),
             (200, hb2), (300, signal0)], ["accepted_session_start", "accepted_latest_good",
             "accepted_session_restart", "accepted_latest_good"])
    run_case("duplicate_after_restart", [(0, hb1), (100, hb2), (200, signal0), (300, signal0)],
             ["accepted_session_start", "accepted_session_restart", "accepted_latest_good",
              "rejected_duplicate"])
    run_case("signal_before_heartbeat", [(0, signal200)], ["rejected_no_live_session"])
    run_case("heartbeat_timeout", [(0, hb1), (2500001, signal200)],
             ["accepted_session_start", "rejected_no_live_session"])
    run_case("same_session_recovery_retains_sequence", [(0, hb1), (100, signal200),
             (2500101, hb1), (2500200, signal200)], ["accepted_session_start",
             "accepted_latest_good", "accepted_session_recovery", "rejected_duplicate"])
    wrong_version = encode_heartbeat(1, 0xA0010001, version=2)
    run_case("unsupported_protocol_version", [(0, wrong_version)], ["rejected_protocol_version"])
    reserved = encode_heartbeat(1, 0xA0010001, reserved=1)
    run_case("heartbeat_reserved_nonzero", [(0, reserved)], ["rejected_reserved"])
    standard = {**signal200, "extended": False}
    run_case("custom_standard_frame_not_protocol_v1", [(0, standard)], ["rejected_frame_kind"])
    old_unbound = {**distance, "sequence": 201}
    run_case("old_signal_bytes_not_session_bound", [(0, hb1), (100, signal200),
             (200, hb2), (300, signal0), (400, encode_signal(old_unbound, contract))],
             ["accepted_session_start", "accepted_latest_good", "accepted_session_restart",
              "accepted_latest_good", "accepted_latest_good"],
             "signal payload has no session_id; producer queue flush is required at restart")

    environment_temp = {"semantic_id": "signal.bench.environment.air_temperature", "sequence": 9,
                        "raw_value": 2345, "validity": 0, "fresh": True, "usable": True}
    environment_humidity = {"semantic_id": "signal.bench.environment.relative_humidity", "sequence": 9,
                            "raw_value": 5678, "validity": 0, "fresh": True, "usable": True}
    hb_c1 = encode_heartbeat(3, 0xC0010001)
    hb_c2 = encode_heartbeat(3, 0xC0010002)
    run_case("one_session_covers_multiple_signals", [(0, hb_c1),
             (100, encode_signal(environment_temp, contract)),
             (200, encode_signal(environment_humidity, contract)), (300, hb_c2),
             (400, encode_signal({**environment_temp, "sequence": 0}, contract)),
             (500, encode_signal({**environment_humidity, "sequence": 0}, contract))],
             ["accepted_session_start", "accepted_latest_good", "accepted_latest_good",
              "accepted_session_restart", "accepted_latest_good", "accepted_latest_good"])
    return cases


def emit_projection(contract: dict[str, Any], observation: dict[str, Any],
                    golden_checks: list[dict[str, Any]]) -> dict[str, Any]:
    wire = emit_wire_projection(contract, observation)
    nominal = run_nominal_receiver(contract, wire)
    cases = conformance_cases(contract)
    return {"schema": "fisiCs.esp_can_protocol_session_projection", "schema_version": 1,
            "project_id": contract["project_id"], "source_inputs": contract["source_inputs"],
            "esp_protocol": contract["esp_protocol"], "node_bindings": contract["node_bindings"],
            "signal_bindings": contract["signal_bindings"], "runtime_policy": contract["runtime_policy"],
            "golden_vector_checks": golden_checks, "wire": wire,
            "nominal_receiver": nominal, "conformance_cases": cases,
            "compatibility": {"pass4_custom_transport": "preserved_but_not_esp_protocol_v1_wire_compatible",
                              "pass5_custom_receiver": "preserved_as_behavioral_predecessor",
                              "protocol_v1_projection": "extended_29_bit_big_endian",
                              "session_restart": "supported_with_heartbeat_and_producer_queue_flush",
                              "old_signal_session_attribution": "not_representable_in_signal_payload_v1"}}


def emit_replay(projection: dict[str, Any]) -> dict[str, Any]:
    nominal = projection["nominal_receiver"]
    restart = next(item for item in projection["conformance_cases"]
                   if item["case_id"] == "session_restart_accepts_sequence_reset")
    residual = next(item for item in projection["conformance_cases"]
                    if item["case_id"] == "old_signal_bytes_not_session_bound")
    return {"schema": "fisiCs.esp_can_protocol_session_replay", "schema_version": 1,
            "protocol_schema_identity_sha256": projection["esp_protocol"]["schema_identity_sha256"],
            "slices": [
                {"slice_id": "initial_sessions", "events": nominal["events"][:8]},
                {"slice_id": "session_restart", "case": restart},
                {"slice_id": "heartbeat_expiry_and_recovery",
                 "cases": [item for item in projection["conformance_cases"]
                           if item["case_id"] in {"heartbeat_timeout", "same_session_recovery_retains_sequence"}]},
                {"slice_id": "residual_session_binding_gap", "case": residual},
            ]}


def validate_projection(contract: dict[str, Any], projection: dict[str, Any],
                        observation: dict[str, Any], golden: list[dict[str, Any]]) -> None:
    require(canonical_bytes(projection) == canonical_bytes(emit_projection(contract, observation, golden)),
            "Pass 6 deterministic projection replay mismatch")
    require(all(item["matched"] for item in projection["golden_vector_checks"]),
            "Pass 6 golden vector mismatch")
    stats = projection["nominal_receiver"]["statistics"]
    require(stats["session_starts"] == 2, "Pass 6 did not establish both source sessions")
    require(stats["heartbeat_refreshes"] > 0, "Pass 6 did not refresh heartbeats")
    require(stats["no_session_rejections"] == 0,
            "Pass 6 released a signal before initial session admission")
    require(stats["latest_good_updates"] == 136 and stats["degraded_updates"] == 3,
            "Pass 6 receiver update classification drift")
    require(stats["duplicate_rejections"] == 10, "Pass 6 duplicate count drift")
    require(stats["offline_transitions"] == 2, "Pass 6 did not expire both sources")
    require(all(item["passed"] for item in projection["conformance_cases"]),
            "Pass 6 conformance case failed")
    require(projection["compatibility"]["old_signal_session_attribution"] ==
            "not_representable_in_signal_payload_v1", "Pass 6 hid residual session gap")


def expect_failure(contract: dict[str, Any], projection: dict[str, Any], observation: dict[str, Any],
                   golden: list[dict[str, Any]], mutate: Callable[[dict[str, Any]], None], label: str) -> str:
    candidate = copy.deepcopy(projection)
    mutate(candidate)
    try:
        validate_projection(contract, candidate, observation, golden)
    except (ProtocolSessionError, KeyError, ValueError):
        return label
    raise ProtocolSessionError(f"negative Pass 6 projection unexpectedly passed: {label}")


def negative_proof(contract: dict[str, Any], projection: dict[str, Any],
                   observation: dict[str, Any], golden: list[dict[str, Any]]) -> list[str]:
    return [
        expect_failure(contract, projection, observation, golden,
                       lambda value: value["source_inputs"].__setitem__(
                           "pass3_observation_sha256", "0" * 64), "wrong_observation_hash_rejected"),
        expect_failure(contract, projection, observation, golden,
                       lambda value: value["esp_protocol"].__setitem__("protocol_version", 2),
                       "protocol_version_drift_rejected"),
        expect_failure(contract, projection, observation, golden,
                       lambda value: value["golden_vector_checks"][0].__setitem__("matched", False),
                       "golden_vector_mismatch_rejected"),
        expect_failure(contract, projection, observation, golden,
                       lambda value: value["nominal_receiver"]["statistics"].__setitem__(
                           "session_starts", 0), "session_start_count_mismatch_rejected"),
        expect_failure(contract, projection, observation, golden,
                       lambda value: value["compatibility"].__setitem__(
                           "old_signal_session_attribution", "supported"),
                       "invented_signal_session_binding_rejected"),
    ]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--esp-work-root", required=True, type=Path)
    parser.add_argument("--fixture-root", type=Path, default=DEFAULT_FIXTURE_ROOT)
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    parser.add_argument("--allow-unpinned", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()
    esp_root = args.esp_work_root.resolve()
    fixture_root = args.fixture_root.resolve()
    build_root = args.build_root.resolve()
    require(fixture_root == DEFAULT_FIXTURE_ROOT.resolve(), "runner accepts only fixed Pass 6 fixture")
    require(build_root == DEFAULT_BUILD_ROOT.resolve(), "runner accepts only fixed Pass 6 build root")
    require((esp_root / ".git").exists(), "ESP root is not a Git checkout")
    require(Path(git(esp_root, "rev-parse", "--show-toplevel").strip()).resolve() == esp_root,
            "ESP root is not the exact Git top level")

    before_status = git(esp_root, "status", "--porcelain=v2", "--branch")
    contract_path = fixture_root / "contract.json"
    contract = load_json(contract_path)
    validate_contract(contract, allow_unpinned=args.allow_unpinned)
    external_inputs = verify_external_inputs(esp_root, contract)
    external_codec_gate = run_external_codec_gate(esp_root)
    observation, _ = validate_prerequisites(contract)
    golden = verify_golden_vectors(esp_root, contract)
    projection = emit_projection(contract, observation, golden)
    validate_projection(contract, projection, observation, golden)
    replay = emit_replay(projection)
    require(canonical_bytes(replay) == canonical_bytes(emit_replay(projection)),
            "Pass 6 replay is not deterministic")
    build_root.mkdir(parents=True, exist_ok=True)
    projection_path = build_root / "protocol-session.json"
    replay_path = build_root / "replay-slices.json"
    canonical_write(projection_path, projection)
    canonical_write(replay_path, replay)
    projection_hash = sha256_path(projection_path)
    replay_hash = sha256_path(replay_path)
    if not args.allow_unpinned:
        require(projection_hash == contract["expected_projection_sha256"],
                "Pass 6 projection hash drift")
        require(replay_hash == contract["expected_replay_sha256"], "Pass 6 replay hash drift")
    negatives = negative_proof(contract, projection, observation, golden)
    require(verify_external_inputs(esp_root, contract) == external_inputs,
            "ESP protocol inputs changed during Pass 6")
    after_status = git(esp_root, "status", "--porcelain=v2", "--branch")
    require(after_status == before_status, "ESP Git status changed during Pass 6")
    stats = projection["nominal_receiver"]["statistics"]
    receipt = {"schema": "fisiCs.esp_can_protocol_session_receipt", "schema_version": 1,
               "project_id": contract["project_id"], "status": "pass",
               "contract": {"path": str(contract_path), "sha256": sha256_path(contract_path)},
               "external_inputs": external_inputs,
               "external_codec_gate": external_codec_gate,
               "protocol_session": {"path": str(projection_path), "sha256": projection_hash,
                                    "byte_deterministic": True},
               "replay_slices": {"path": str(replay_path), "sha256": replay_hash,
                                 "byte_deterministic": True, "count": len(replay["slices"])},
               "wire_statistics": projection["wire"]["statistics"],
               "receiver_statistics": stats,
               "golden_vectors": {"checked": len(golden), "matched": True},
               "conformance_cases": {"count": len(projection["conformance_cases"]), "passed": True},
               "negative_proof": negatives,
               "compatibility": projection["compatibility"],
               "claim_boundary": {"esp_protocol_v1_wire": "source_and_golden_vector_compatible",
                                  "heartbeat_sessions": "deterministically_simulated",
                                  "session_restart": "deterministically_simulated_with_queue_flush_requirement",
                                  "signal_session_attribution": "not_representable_in_protocol_v1",
                                  "generated_cpp_codec": "source_digest_and_golden_fixture_bound",
                                  "firmware_build": "not_assessed", "deployment": "not_assessed",
                                  "runtime_hardware_observation": "not_assessed",
                                  "physical_acceptance": "not_assessed"},
               "external_repository": {"path": str(esp_root),
                                       "head": git(esp_root, "rev-parse", "HEAD").strip(),
                                       "status_sha256": hashlib.sha256(before_status.encode()).hexdigest(),
                                       "git_status_unchanged": True}}
    canonical_write(build_root / "receipt.json", receipt)
    print("ESP CAN Pass 6=PASS "
          f"signals={projection['wire']['statistics']['signal_frames']} "
          f"heartbeats={projection['wire']['statistics']['heartbeat_frames']} "
          f"sessions={stats['session_starts']} refreshes={stats['heartbeat_refreshes']} "
          f"cases={len(projection['conformance_cases'])} negatives={len(negatives)} "
          "external_tree=UNCHANGED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ProtocolSessionError as exc:
        print(f"ESP CAN Pass 6: FAIL: {exc}", file=os.sys.stderr)
        raise SystemExit(1)
