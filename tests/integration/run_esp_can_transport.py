#!/usr/bin/env python3
"""Pass 4 deterministic Classical CAN frame, queue, and transport projection."""

from __future__ import annotations

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
from typing import Any, Callable


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_FIXTURE_ROOT = FISICS_ROOT / "tests/fixtures/esp_can_transport_v1"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-can-transport"
DEFAULT_OBSERVATION = FISICS_ROOT / "build/integration-esp-can-simulation/simulated-observation.json"
DEFAULT_PASS3_RECEIPT = FISICS_ROOT / "build/integration-esp-can-simulation/receipt.json"
CRC15_POLYNOMIAL = 0x4599


class TransportError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise TransportError(message)


def load_json(path: Path) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise TransportError(f"cannot read JSON {path}: {exc}") from exc
    require(isinstance(value, dict), f"JSON root must be an object: {path}")
    return value


def canonical_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def canonical_write(path: Path, value: Any) -> None:
    path.write_bytes(canonical_bytes(value))


def sha256_path(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def ceil_quantum(value: int, quantum: int) -> int:
    return ((value + quantum - 1) // quantum) * quantum


def integer_bits(value: int, width: int) -> list[int]:
    require(0 <= value < (1 << width), f"value {value} does not fit unsigned {width}-bit field")
    return [(value >> shift) & 1 for shift in range(width - 1, -1, -1)]


def crc15(bits: list[int]) -> int:
    remainder = 0
    for bit in bits:
        feedback = ((remainder >> 14) & 1) ^ bit
        remainder = (remainder << 1) & 0x7FFF
        if feedback:
            remainder ^= CRC15_POLYNOMIAL
    return remainder


def stuff_bits(bits: list[int]) -> tuple[list[int], int]:
    require(bool(bits), "cannot stuff an empty frame prefix")
    output = [bits[0]]
    previous = bits[0]
    run_length = 1
    inserted = 0
    for bit in bits[1:]:
        output.append(bit)
        if bit == previous:
            run_length += 1
        else:
            previous = bit
            run_length = 1
        if run_length == 5:
            stuffed = 1 - bit
            output.append(stuffed)
            inserted += 1
            previous = stuffed
            run_length = 1
    return output, inserted


def encode_payload(record: dict[str, Any]) -> bytes:
    sequence = record["sequence"]
    raw = record["raw_value"]
    quality = record["quality"]
    require(type(sequence) is int and 0 <= sequence <= 0xFFFF, "sequence exceeds u16 payload")
    require(type(raw) is int and -(1 << 31) <= raw < (1 << 31), "raw value exceeds i32 payload")
    require(type(quality) is int and 0 <= quality <= 0xFF, "quality exceeds u8 payload")
    status = ((1 if record["validity"] == 0 else 0)
              | ((1 if record["fresh"] else 0) << 1)
              | ((1 if record["usable"] else 0) << 2))
    return (sequence.to_bytes(2, "little") + raw.to_bytes(4, "little", signed=True)
            + bytes((status, quality)))


def decode_payload(payload: bytes) -> dict[str, int | bool]:
    require(len(payload) == 8, "payload is not eight bytes")
    status = payload[6]
    require((status & 0xF8) == 0, "reserved status bits are set")
    return {
        "sequence": int.from_bytes(payload[0:2], "little"),
        "raw_value": int.from_bytes(payload[2:6], "little", signed=True),
        "valid": bool(status & 0x01),
        "fresh": bool(status & 0x02),
        "usable": bool(status & 0x04),
        "quality": payload[7],
    }


def frame_metrics(can_id: int, payload: bytes) -> dict[str, Any]:
    require(0 <= can_id <= 0x7FF, "CAN identifier exceeds standard 11-bit range")
    require(len(payload) == 8, "Pass 4 requires DLC 8")
    # SOF, 11-bit identifier, RTR, IDE, r0, DLC, data. CRC covers this destuffed prefix.
    crc_input = ([0] + integer_bits(can_id, 11) + [0, 0, 0]
                 + integer_bits(len(payload), 4)
                 + [bit for byte in payload for bit in integer_bits(byte, 8)])
    checksum = crc15(crc_input)
    stuffed_prefix, stuff_count = stuff_bits(crc_input + integer_bits(checksum, 15))
    # CRC delimiter, ACK slot, ACK delimiter, EOF, and intermission are outside stuffing.
    total_bits = len(stuffed_prefix) + 1 + 1 + 1 + 7 + 3
    return {
        "crc15": f"0x{checksum:04x}",
        "stuff_bits": stuff_count,
        "unstuffed_bits_including_intermission": 111,
        "total_wire_bits": total_bits,
    }


def validate_contract(contract: dict[str, Any], *, allow_unpinned: bool = False) -> None:
    require(set(contract) == {
        "schema", "schema_version", "project_id", "source_observation", "frame_format",
        "payload_codec", "routes", "bus", "scenarios", "expected_transport_sha256",
    }, "unexpected Pass 4 contract fields")
    require(contract["schema"] == "fisiCs.esp_can_transport_contract",
            "unsupported Pass 4 contract schema")
    require(contract["schema_version"] == 1, "unsupported Pass 4 contract version")
    require(contract["project_id"] == "esp.can_pass2.sensor_backbone", "Pass 4 project drift")
    require(contract["source_observation"] == {
        "schema": "fisiCs.esp_can_simulated_observation",
        "sha256": "e142643a7a361431ceac65e38a9b595be58c81bc33e26ad74e5687b922581a35",
        "records": 149,
    }, "Pass 4 source observation contract drift")
    require(contract["frame_format"] == {
        "name": "classical_can_2_0a_standard_data_frame", "identifier_bits": 11,
        "dlc": 8, "crc": "can_crc15_poly_0x4599",
        "bit_stuffing": "sof_through_crc_sequence_after_five_equal_bits",
        "acknowledgement": "modeled_outcome_not_electrical_dominance",
    }, "Pass 4 frame format drift")
    require(contract["payload_codec"] == {
        "name": "fisiCs.sensor_scalar_le_v1", "size_bytes": 8,
        "layout": ["sequence:u16le", "raw_value:i32le",
                   "status:u8(valid_bit0,fresh_bit1,usable_bit2)", "quality:u8"],
    }, "Pass 4 payload codec drift")
    routes = contract["routes"]
    require(len(routes) == 3, "Pass 4 requires exactly three routes")
    semantic_ids = [item["semantic_id"] for item in routes]
    can_ids = [item["can_id"] for item in routes]
    require(len(set(semantic_ids)) == len(semantic_ids), "duplicate Pass 4 semantic route")
    require(len(set(can_ids)) == len(can_ids), "duplicate Pass 4 CAN identifier")
    require(all(type(value) is int and 0 <= value <= 0x7FF for value in can_ids),
            "invalid Pass 4 CAN identifier")
    require(all(set(item) == {"semantic_id", "can_id", "producer_node", "consumer_node"}
                for item in routes), "unexpected Pass 4 route fields")
    require(contract["bus"] == {
        "bus_id": "bus.can.sensor_backbone", "bitrate": 500000,
        "publication_quantum_us": 1000000,
        "queue_policy": "per_node_fifo_drop_newest",
        "arbitration_policy": "lowest_standard_identifier_wins_between_node_heads",
        "retry_policy": "immediate_after_intermission_reenter_arbitration",
    }, "Pass 4 bus contract drift")
    require({item["scenario_id"] for item in contract["scenarios"]} == {
        "baseline_transient_ack_loss", "constrained_queue_persistent_ack_loss"
    }, "Pass 4 scenario set drift")
    for scenario in contract["scenarios"]:
        require(set(scenario) == {"scenario_id", "queue_capacity_by_node",
                                  "max_attempts", "faults"},
                "unexpected Pass 4 scenario fields")
        require(set(scenario["queue_capacity_by_node"]) == {
            "node.can.distance_endpoint", "node.can.environment_middle"
        }, "Pass 4 scenario queue-node set drift")
        require(type(scenario["max_attempts"]) is int and 1 <= scenario["max_attempts"] <= 8,
                "invalid maximum attempt count")
        require(all(type(value) is int and 1 <= value <= 64
                    for value in scenario["queue_capacity_by_node"].values()),
                "invalid queue capacity")
        for fault in scenario["faults"]:
            require(fault.get("kind") in {"ack_loss_once", "ack_loss_always"},
                    "unsupported Pass 4 fault kind")
            expected_fields = ({"kind", "semantic_id", "sequence", "attempt"}
                               if fault["kind"] == "ack_loss_once"
                               else {"kind", "semantic_id", "sequence"})
            require(set(fault) == expected_fields, "unexpected Pass 4 fault fields")
            require(fault["semantic_id"] in semantic_ids, "fault uses unknown semantic identity")
            require(type(fault["sequence"]) is int and fault["sequence"] >= 0,
                    "fault sequence must be a non-negative integer")
    if not allow_unpinned:
        digest = contract["expected_transport_sha256"]
        require(type(digest) is str and len(digest) == 64,
                "Pass 4 transport output is not pinned")


def validate_source(contract: dict[str, Any], observation_path: Path,
                    pass3_receipt_path: Path) -> dict[str, Any]:
    require(sha256_path(observation_path) == contract["source_observation"]["sha256"],
            "Pass 3 simulated observation hash mismatch")
    observation = load_json(observation_path)
    require(observation.get("schema") == contract["source_observation"]["schema"],
            "Pass 3 simulated observation schema mismatch")
    require(observation.get("project_id") == contract["project_id"],
            "Pass 3 simulated observation project mismatch")
    require(len(observation.get("records", [])) == contract["source_observation"]["records"],
            "Pass 3 simulated observation record count mismatch")
    receipt = load_json(pass3_receipt_path)
    require(receipt.get("status") == "pass", "Pass 3 receipt is not passing")
    require(receipt.get("simulated_observation", {}).get("sha256")
            == contract["source_observation"]["sha256"], "Pass 3 receipt join mismatch")
    return observation


def make_frames(contract: dict[str, Any], observation: dict[str, Any]) -> list[dict[str, Any]]:
    routes = {item["semantic_id"]: item for item in contract["routes"]}
    quantum = contract["bus"]["publication_quantum_us"]
    frames = []
    for index, record in enumerate(observation["records"]):
        require(record["semantic_id"] in routes, f"unmapped source record {index}")
        route = routes[record["semantic_id"]]
        require(record["producer_node"] == route["producer_node"],
                f"producer node drift in source record {index}")
        require(record["consumer_node"] == route["consumer_node"],
                f"consumer node drift in source record {index}")
        payload = encode_payload(record)
        decoded = decode_payload(payload)
        require(decoded == {
            "sequence": record["sequence"], "raw_value": record["raw_value"],
            "valid": record["validity"] == 0, "fresh": record["fresh"],
            "usable": record["usable"], "quality": record["quality"],
        }, f"payload round-trip mismatch for source record {index}")
        metrics = frame_metrics(route["can_id"], payload)
        frames.append({
            "frame_index": index,
            "source_record_index": record["source_record_index"],
            "source_profile": record["source_profile"],
            "source_at_us": record["at_us"],
            "release_at_us": ceil_quantum(record["at_us"], quantum),
            "semantic_id": record["semantic_id"],
            "producer_node": route["producer_node"],
            "consumer_node": route["consumer_node"],
            "can_id": route["can_id"],
            "can_id_hex": f"0x{route['can_id']:03x}",
            "dlc": 8,
            "payload_hex": payload.hex(),
            "decoded_payload": decoded,
            **metrics,
        })
    frames.sort(key=lambda item: (item["release_at_us"], item["source_at_us"],
                                  item["semantic_id"], item["frame_index"]))
    return frames


def should_fail(frame: dict[str, Any], attempt: int, faults: list[dict[str, Any]]) -> bool:
    for fault in faults:
        if (fault["semantic_id"] == frame["semantic_id"]
                and fault["sequence"] == frame["decoded_payload"]["sequence"]):
            if fault["kind"] == "ack_loss_always":
                return True
            if fault["kind"] == "ack_loss_once" and fault["attempt"] == attempt:
                return True
    return False


def simulate_scenario(contract: dict[str, Any], frames: list[dict[str, Any]],
                      scenario: dict[str, Any]) -> dict[str, Any]:
    bitrate = contract["bus"]["bitrate"]
    capacities = scenario["queue_capacity_by_node"]
    queues: dict[str, list[dict[str, Any]]] = {node: [] for node in capacities}
    high_water = {node: 0 for node in capacities}
    arbitration_losses = {node: 0 for node in capacities}
    attempts: list[dict[str, Any]] = []
    drops: list[dict[str, Any]] = []
    delivered: list[dict[str, Any]] = []
    attempt_counts: dict[int, int] = {}
    cursor = 0
    now_us = 0

    while cursor < len(frames) or any(queues.values()):
        if not any(queues.values()) and cursor < len(frames):
            now_us = max(now_us, frames[cursor]["release_at_us"])
        while cursor < len(frames) and frames[cursor]["release_at_us"] <= now_us:
            frame = frames[cursor]
            node = frame["producer_node"]
            require(node in queues, f"scenario has no queue for {node}")
            if len(queues[node]) >= capacities[node]:
                drops.append({
                    "frame_index": frame["frame_index"], "semantic_id": frame["semantic_id"],
                    "producer_node": node, "at_us": now_us, "reason": "queue_overflow",
                })
            else:
                queues[node].append(frame)
                high_water[node] = max(high_water[node], len(queues[node]))
            cursor += 1

        candidates = [queue[0] for queue in queues.values() if queue]
        require(bool(candidates), "transport scheduler has no candidate")
        winner = min(candidates, key=lambda item: (item["can_id"], item["frame_index"]))
        losers = [item for item in candidates if item is not winner]
        for loser in losers:
            arbitration_losses[loser["producer_node"]] += 1
        frame_index = winner["frame_index"]
        attempt = attempt_counts.get(frame_index, 0) + 1
        attempt_counts[frame_index] = attempt
        duration_us = (winner["total_wire_bits"] * 1_000_000 + bitrate - 1) // bitrate
        end_us = now_us + duration_us
        failed = should_fail(winner, attempt, scenario["faults"])
        if failed and attempt < scenario["max_attempts"]:
            outcome = "ack_loss_retry"
        elif failed:
            outcome = "ack_loss_exhausted"
            queues[winner["producer_node"]].pop(0)
            drops.append({
                "frame_index": frame_index, "semantic_id": winner["semantic_id"],
                "producer_node": winner["producer_node"], "at_us": end_us,
                "reason": "retry_exhausted", "attempts": attempt,
            })
        else:
            outcome = "delivered"
            queues[winner["producer_node"]].pop(0)
            delivered.append({
                "frame_index": frame_index, "semantic_id": winner["semantic_id"],
                "consumer_node": winner["consumer_node"], "delivered_at_us": end_us,
                "transport_latency_us": end_us - winner["release_at_us"],
                "source_to_delivery_us": end_us - winner["source_at_us"],
                "attempts": attempt,
            })
        attempts.append({
            "attempt_index": len(attempts), "frame_index": frame_index,
            "semantic_id": winner["semantic_id"], "producer_node": winner["producer_node"],
            "can_id": winner["can_id"], "payload_hex": winner["payload_hex"],
            "crc15": winner["crc15"], "stuff_bits": winner["stuff_bits"],
            "total_wire_bits": winner["total_wire_bits"], "attempt": attempt,
            "start_at_us": now_us, "end_at_us": end_us, "outcome": outcome,
            "arbitration_loser_frame_indices": sorted(item["frame_index"] for item in losers),
        })
        now_us = end_us

    wire_bits = sum(item["total_wire_bits"] for item in attempts)
    frame_lookup = frames_by_index(frames)
    delivered_bits = sum(frame_lookup[item["frame_index"]]["total_wire_bits"]
                         for item in delivered)
    horizon_us = max(now_us, max(item["release_at_us"] for item in frames))
    utilization_denominator = horizon_us * bitrate
    latency_values = [item["transport_latency_us"] for item in delivered]
    source_latency_values = [item["source_to_delivery_us"] for item in delivered]
    return {
        "scenario_id": scenario["scenario_id"],
        "configuration": {
            "queue_capacity_by_node": capacities,
            "max_attempts": scenario["max_attempts"],
            "faults": scenario["faults"],
        },
        "attempts": attempts,
        "deliveries": delivered,
        "drops": drops,
        "statistics": {
            "source_frames": len(frames),
            "transmit_attempts": len(attempts),
            "retry_attempts": sum(max(0, count - 1) for count in attempt_counts.values()),
            "delivered_frames": len(delivered),
            "queue_overflow_drops": sum(item["reason"] == "queue_overflow" for item in drops),
            "retry_exhausted_drops": sum(item["reason"] == "retry_exhausted" for item in drops),
            "queue_high_water_by_node": high_water,
            "arbitration_losses_by_node": arbitration_losses,
            "arbitration_loss_events": sum(arbitration_losses.values()),
            "wire_bits_attempted": wire_bits,
            "wire_bits_delivered": delivered_bits,
            "simulation_horizon_us": horizon_us,
            "utilization": {
                "numerator_wire_bits_x_1000000us": wire_bits * 1_000_000,
                "denominator_horizon_us_x_bitrate": utilization_denominator,
                "parts_per_million_floor": (wire_bits * 1_000_000_000_000)
                                           // utilization_denominator,
            },
            "transport_latency_us": latency_summary(latency_values),
            "source_to_delivery_us": latency_summary(source_latency_values),
        },
    }


def frames_by_index(frames: list[dict[str, Any]]) -> dict[int, dict[str, Any]]:
    return {item["frame_index"]: item for item in frames}


def latency_summary(values: list[int]) -> dict[str, int]:
    require(bool(values), "cannot summarize empty latency values")
    return {
        "minimum": min(values), "maximum": max(values),
        "mean_numerator": sum(values), "mean_denominator": len(values),
    }


def emit_transport(contract: dict[str, Any], observation: dict[str, Any]) -> dict[str, Any]:
    frames = make_frames(contract, observation)
    scenarios = [simulate_scenario(contract, frames, scenario)
                 for scenario in contract["scenarios"]]
    return {
        "schema": "fisiCs.esp_can_transport_projection",
        "schema_version": 1,
        "project_id": contract["project_id"],
        "source_observation": contract["source_observation"],
        "frame_format": contract["frame_format"],
        "payload_codec": contract["payload_codec"],
        "bus": contract["bus"],
        "routes": contract["routes"],
        "frames": frames,
        "scenarios": scenarios,
    }


def validate_transport(contract: dict[str, Any], transport: dict[str, Any]) -> None:
    require(transport.get("source_observation") == contract["source_observation"],
            "transport source-observation join mismatch")
    require(transport.get("frame_format") == contract["frame_format"],
            "transport frame format mismatch")
    require(transport.get("payload_codec") == contract["payload_codec"],
            "transport payload codec mismatch")
    route_by_semantic = {item["semantic_id"]: item for item in contract["routes"]}
    frame_index = frames_by_index(transport["frames"])
    require(len(frame_index) == contract["source_observation"]["records"],
            "transport frame count mismatch")
    for frame in transport["frames"]:
        route = route_by_semantic[frame["semantic_id"]]
        require(frame["can_id"] == route["can_id"], "frame CAN identifier mismatch")
        payload = bytes.fromhex(frame["payload_hex"])
        require(frame["decoded_payload"] == decode_payload(payload), "payload decode mismatch")
        metrics = frame_metrics(frame["can_id"], payload)
        for key, value in metrics.items():
            require(frame[key] == value, f"frame {key} mismatch")
        require(frame["release_at_us"] >= frame["source_at_us"], "frame releases before source")

    scenario_by_id = {item["scenario_id"]: item for item in transport["scenarios"]}
    scenario_contract_by_id = {item["scenario_id"]: item for item in contract["scenarios"]}
    require(set(scenario_by_id) == set(scenario_contract_by_id),
            "transport scenario coverage mismatch")
    for scenario in transport["scenarios"]:
        require(canonical_bytes(scenario) == canonical_bytes(simulate_scenario(
            contract, transport["frames"], scenario_contract_by_id[scenario["scenario_id"]])),
            f"transport scenario replay mismatch for {scenario['scenario_id']}")
        attempts_by_frame: dict[int, list[dict[str, Any]]] = {}
        for attempt in scenario["attempts"]:
            require(attempt["frame_index"] in frame_index, "attempt refers to unknown frame")
            frame = frame_index[attempt["frame_index"]]
            require(attempt["can_id"] == frame["can_id"], "attempt CAN identifier mismatch")
            require(attempt["payload_hex"] == frame["payload_hex"], "attempt payload mismatch")
            require(attempt["crc15"] == frame["crc15"], "attempt CRC mismatch")
            require(attempt["end_at_us"] > attempt["start_at_us"], "non-positive frame duration")
            attempts_by_frame.setdefault(attempt["frame_index"], []).append(attempt)
        for attempts in attempts_by_frame.values():
            require([item["attempt"] for item in attempts] == list(range(1, len(attempts) + 1)),
                    "retry attempt sequence is not contiguous")
        delivered_indices = {item["frame_index"] for item in scenario["deliveries"]}
        dropped_indices = {item["frame_index"] for item in scenario["drops"]}
        require(delivered_indices.isdisjoint(dropped_indices), "frame both delivered and dropped")
        require(len(delivered_indices | dropped_indices) == len(frame_index),
                "scenario does not resolve every source frame")
        for delivery in scenario["deliveries"]:
            require(delivery["transport_latency_us"] >= 0, "negative transport latency")
            require(delivery["source_to_delivery_us"] >= delivery["transport_latency_us"],
                    "end-to-end latency is below transport latency")
        stats = scenario["statistics"]
        require(stats["source_frames"] == len(frame_index), "statistics source count mismatch")
        require(stats["transmit_attempts"] == len(scenario["attempts"]),
                "statistics attempt count mismatch")
        require(stats["delivered_frames"] == len(scenario["deliveries"]),
                "statistics delivery count mismatch")
        require(stats["queue_overflow_drops"]
                == sum(item["reason"] == "queue_overflow" for item in scenario["drops"]),
                "statistics overflow count mismatch")
        require(stats["retry_exhausted_drops"]
                == sum(item["reason"] == "retry_exhausted" for item in scenario["drops"]),
                "statistics retry-exhaustion count mismatch")
        utilization = stats["utilization"]
        expected_ppm = (stats["wire_bits_attempted"] * 1_000_000_000_000
                        // (stats["simulation_horizon_us"] * contract["bus"]["bitrate"]))
        require(utilization["parts_per_million_floor"] == expected_ppm,
                "bus utilization calculation mismatch")

    baseline = scenario_by_id["baseline_transient_ack_loss"]["statistics"]
    require(baseline["delivered_frames"] == len(frame_index), "baseline did not deliver all frames")
    require(baseline["queue_overflow_drops"] == 0, "baseline queue overflowed")
    require(baseline["retry_exhausted_drops"] == 0, "baseline exhausted retry")
    require(baseline["retry_attempts"] == 1, "baseline transient loss did not retry exactly once")
    require(baseline["arbitration_loss_events"] > 0, "baseline did not exercise arbitration")
    stress = scenario_by_id["constrained_queue_persistent_ack_loss"]["statistics"]
    require(stress["queue_overflow_drops"] > 0, "constrained scenario did not exercise capacity")
    require(stress["retry_exhausted_drops"] == 1,
            "constrained scenario did not exhaust exactly one retry")


def expect_failure(contract: dict[str, Any], transport: dict[str, Any],
                   mutate: Callable[[dict[str, Any]], None], label: str) -> str:
    candidate = copy.deepcopy(transport)
    mutate(candidate)
    try:
        validate_transport(contract, candidate)
    except (TransportError, KeyError, ValueError):
        return label
    raise TransportError(f"negative transport unexpectedly passed: {label}")


def run_negative_proof(contract: dict[str, Any], transport: dict[str, Any]) -> list[str]:
    failures = [
        expect_failure(contract, transport,
                       lambda value: value["source_observation"].__setitem__("sha256", "0" * 64),
                       "wrong_source_observation_hash_rejected"),
        expect_failure(contract, transport,
                       lambda value: value["frames"][0].__setitem__(
                           "payload_hex", "ff" + value["frames"][0]["payload_hex"][2:]),
                       "payload_corruption_rejected"),
        expect_failure(contract, transport,
                       lambda value: value["frames"][0].__setitem__("crc15", "0x0000"),
                       "crc_mismatch_rejected"),
        expect_failure(contract, transport,
                       lambda value: value["scenarios"][0]["attempts"][0].__setitem__("can_id", 0x7FF),
                       "attempt_identifier_mismatch_rejected"),
        expect_failure(contract, transport,
                       lambda value: value["scenarios"][0]["deliveries"][0].__setitem__(
                           "transport_latency_us", -1),
                       "negative_latency_rejected"),
        expect_failure(contract, transport,
                       lambda value: value["scenarios"][0]["statistics"]["utilization"].__setitem__(
                           "parts_per_million_floor", 999999),
                       "utilization_mismatch_rejected"),
        expect_failure(contract, transport,
                       lambda value: value["scenarios"][0]["attempts"][1].__setitem__("attempt", 7),
                       "retry_sequence_mismatch_rejected"),
    ]
    duplicate_id = copy.deepcopy(contract)
    duplicate_id["routes"][1]["can_id"] = duplicate_id["routes"][0]["can_id"]
    bad_capacity = copy.deepcopy(contract)
    bad_capacity["scenarios"][0]["queue_capacity_by_node"]["node.can.distance_endpoint"] = 0
    for candidate, label in (
        (duplicate_id, "duplicate_can_identifier_rejected"),
        (bad_capacity, "invalid_queue_capacity_rejected"),
    ):
        try:
            validate_contract(candidate)
        except TransportError:
            failures.append(label)
        else:
            raise TransportError(f"negative contract unexpectedly passed: {label}")
    return failures


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture-root", type=Path, default=DEFAULT_FIXTURE_ROOT)
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    parser.add_argument("--observation", type=Path, default=DEFAULT_OBSERVATION)
    parser.add_argument("--pass3-receipt", type=Path, default=DEFAULT_PASS3_RECEIPT)
    parser.add_argument("--allow-unpinned", action="store_true",
                        help=argparse.SUPPRESS)
    args = parser.parse_args()

    fixture_root = args.fixture_root.resolve()
    build_root = args.build_root.resolve()
    observation_path = args.observation.resolve()
    receipt_path = args.pass3_receipt.resolve()
    require(fixture_root == DEFAULT_FIXTURE_ROOT.resolve(), "runner accepts only fixed Pass 4 fixture")
    require(build_root == DEFAULT_BUILD_ROOT.resolve(), "runner accepts only fixed Pass 4 build root")
    require(observation_path == DEFAULT_OBSERVATION.resolve(),
            "runner accepts only the fixed Pass 3 observation path")
    require(receipt_path == DEFAULT_PASS3_RECEIPT.resolve(),
            "runner accepts only the fixed Pass 3 receipt path")

    contract_path = fixture_root / "contract.json"
    contract = load_json(contract_path)
    validate_contract(contract, allow_unpinned=args.allow_unpinned)
    observation = validate_source(contract, observation_path, receipt_path)
    transport = emit_transport(contract, observation)
    validate_transport(contract, transport)
    require(canonical_bytes(transport) == canonical_bytes(emit_transport(contract, observation)),
            "Pass 4 transport projection is not byte deterministic")
    build_root.mkdir(parents=True, exist_ok=True)
    transport_path = build_root / "transport.json"
    canonical_write(transport_path, transport)
    transport_hash = sha256_path(transport_path)
    if not args.allow_unpinned:
        require(transport_hash == contract["expected_transport_sha256"],
                "Pass 4 transport projection hash drift")
    negatives = run_negative_proof(contract, transport)
    scenarios = {item["scenario_id"]: item["statistics"] for item in transport["scenarios"]}
    receipt = {
        "schema": "fisiCs.esp_can_transport_receipt",
        "schema_version": 1,
        "project_id": contract["project_id"],
        "status": "pass",
        "contract": {"path": str(contract_path), "sha256": sha256_path(contract_path)},
        "source_observation": {
            "path": str(observation_path), "sha256": sha256_path(observation_path),
            "pass3_receipt_path": str(receipt_path), "pass3_receipt_sha256": sha256_path(receipt_path),
        },
        "transport": {
            "path": str(transport_path), "sha256": transport_hash,
            "byte_deterministic": True, "source_frames": len(transport["frames"]),
        },
        "scenarios": scenarios,
        "negative_proof": negatives,
        "claim_boundary": {
            "source_sensor_behavior": "host_simulation_input",
            "frame_encoding": "deterministically_simulated",
            "payload_round_trip": "validated",
            "arbitration": "deterministically_simulated_between_node_queue_heads",
            "queue_capacity": "deterministically_simulated",
            "latency": "simulation_time_only",
            "loss_and_retry": "fault_injected_simulation",
            "bus_utilization": "modeled_wire_bits_over_simulation_horizon",
            "esp_twai_driver": "not_assessed",
            "electrical_can": "not_assessed",
            "runtime_hardware_observation": "not_assessed",
            "deployment": "not_assessed",
            "physical_acceptance": "not_assessed"
        },
    }
    canonical_write(build_root / "receipt.json", receipt)
    baseline = scenarios["baseline_transient_ack_loss"]
    stress = scenarios["constrained_queue_persistent_ack_loss"]
    print("ESP CAN Pass 4=PASS "
          f"frames={len(transport['frames'])} baseline_delivered={baseline['delivered_frames']} "
          f"baseline_retries={baseline['retry_attempts']} arbitration_losses={baseline['arbitration_loss_events']} "
          f"stress_overflow={stress['queue_overflow_drops']} stress_retry_exhausted={stress['retry_exhausted_drops']} "
          f"negatives={len(negatives)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except TransportError as exc:
        print(f"ESP CAN Pass 4: FAIL: {exc}", file=os.sys.stderr)
        raise SystemExit(1)
