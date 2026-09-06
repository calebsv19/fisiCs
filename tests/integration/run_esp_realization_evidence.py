#!/usr/bin/env python3
"""Build the fixed E016 Slice F retained realization/evidence receipt."""

from __future__ import annotations

import argparse
import json
import os
import sys
import tempfile
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_esp_portable_c as intake


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONTRACT = FISICS_ROOT / "tests/fixtures/esp_portable_c/e016_realization_v1.json"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build/integration-esp-portable-c"


def load_realization_contract(path: Path) -> dict[str, Any]:
    data = intake.exact_keys(
        intake.load_json(path),
        {
            "schema", "schema_version", "lane_id", "slice_e", "external_evidence",
            "candidates", "shared_source", "static_artifacts", "expected_projection",
        },
        "E016 realization contract",
    )
    intake.require(data["schema"] == "fisiCs.esp_realization_evidence", "unsupported E016 realization schema")
    intake.require(data["schema_version"] == 1, "unsupported E016 realization schema version")
    intake.require(data["lane_id"] == "esp.e016.realization_evidence.v1", "E016 realization lane drift")

    slice_e = intake.exact_keys(data["slice_e"], {"contract_path", "contract_sha256", "receipt_path"}, "slice_e")
    intake.require(intake.relative_path(slice_e["contract_path"], "slice_e.contract_path") == "tests/fixtures/esp_portable_c/e016_proof_v1.json", "Slice F must consume the fixed Slice E contract")
    intake.digest_string(slice_e["contract_sha256"], "slice_e.contract_sha256")
    intake.require(intake.relative_path(slice_e["receipt_path"], "slice_e.receipt_path") == "build/integration-esp-portable-c/e016.deterministic-proof.receipt.json", "Slice E receipt path drift")

    evidence_roles = {"portable_core_evidence", "physical_can_milestone"}
    intake.require(isinstance(data["external_evidence"], list) and len(data["external_evidence"]) == 2, "external evidence count drift")
    seen_roles: set[str] = set()
    for index, item in enumerate(data["external_evidence"]):
        item = intake.exact_keys(item, {"role", "path", "sha256"}, f"external_evidence[{index}]")
        role = intake.nonempty_string(item["role"], f"external_evidence[{index}].role")
        intake.relative_path(item["path"], f"external_evidence[{index}].path")
        intake.digest_string(item["sha256"], f"external_evidence[{index}].sha256")
        intake.require(role not in seen_roles, "duplicate external evidence role")
        seen_roles.add(role)
    intake.require(seen_roles == evidence_roles, "external evidence roles drift")

    expected_roles = {"bench_a_pot_observer", "bench_b_pot_publisher"}
    intake.require(isinstance(data["candidates"], list) and len(data["candidates"]) == 2, "candidate count drift")
    candidate_roles: set[str] = set()
    for index, candidate in enumerate(data["candidates"]):
        candidate = intake.exact_keys(candidate, {"role", "manifest_path", "manifest_sha256", "source_set_sha256", "firmware_version", "expected_usb_serial"}, f"candidates[{index}]")
        role = intake.nonempty_string(candidate["role"], f"candidates[{index}].role")
        intake.relative_path(candidate["manifest_path"], f"candidates[{index}].manifest_path")
        intake.digest_string(candidate["manifest_sha256"], f"candidates[{index}].manifest_sha256")
        intake.digest_string(candidate["source_set_sha256"], f"candidates[{index}].source_set_sha256")
        intake.nonempty_string(candidate["firmware_version"], f"candidates[{index}].firmware_version")
        intake.require(len(candidate["expected_usb_serial"].split(":")) == 6, "candidate USB identity is malformed")
        intake.require(role not in candidate_roles, "duplicate candidate role")
        candidate_roles.add(role)
    intake.require(candidate_roles == expected_roles, "candidate roles drift")

    intake.require(isinstance(data["shared_source"], list) and len(data["shared_source"]) == 3, "shared source count drift")
    for index, item in enumerate(data["shared_source"]):
        item = intake.exact_keys(item, {"path", "sha256"}, f"shared_source[{index}]")
        intake.relative_path(item["path"], f"shared_source[{index}].path")
        intake.digest_string(item["sha256"], f"shared_source[{index}].sha256")

    static_keys = {
        "translation_unit_metadata_sha256", "semantic_project_graph_sha256",
        "physical_composition_graph_sha256", "hardware_compatibility_graph_sha256",
    }
    static_artifacts = intake.exact_keys(data["static_artifacts"], static_keys, "static_artifacts")
    for key, value in static_artifacts.items():
        intake.digest_string(value, f"static_artifacts.{key}")

    frozen_projection = {
        "bench_a_selected_candidate_upload": "documented_consistent",
        "bench_b_selected_candidate_upload": "conflict_unresolved",
        "shared_portable_source": "exact_match",
        "candidate_protocol_schema": "exact_match",
        "current_protocol_schema": "drift_reported",
        "initial_no_peer_bus_off": "documented",
        "recovered_transport_pongs": "118..135",
        "recovered_semantic_sequences": "148..183",
        "recovered_signal_quality": "GOOD",
        "operator_potentiometer_response": "documented",
        "calibration": "not_assessed",
        "fresh_runtime_state": "not_assessed",
    }
    intake.require(data["expected_projection"] == frozen_projection, "E016 realization projection drift or evidence promotion")
    return data


def verify_evidence_text(portable: str, milestone: str, candidates: dict[str, dict[str, Any]]) -> dict[str, Any]:
    bench_a = candidates["bench_a_pot_observer"]
    bench_b = candidates["bench_b_pot_publisher"]
    for value in (bench_a["manifest_contract_path"], bench_a["source_set_sha256"], bench_b["manifest_contract_path"], bench_b["source_set_sha256"]):
        intake.require(value in portable, f"portable evidence does not bind selected value {value}")
    required_portable = (
        "uploaded to `/dev/cu.usbmodem1301`",
        "matching Bench A",
        "matched USB\nserial `20:6E:F1:19:8B:1C`",
        "`ERROR_PASSIVE` -> `BUS_OFF` -> `BUS_RECOVERED`",
        "No pong or semantic frame appeared",
        "continuous transport pongs 118 through 135",
        "semantic sequences 148 through 183",
        "`quality=GOOD` with update age returning to zero",
        "potentiometer response and visible\nbehavior were working properly",
    )
    for statement in required_portable:
        intake.require(statement in portable, f"portable retained evidence statement drift: {statement}")
    required_milestone = (
        "Bench A 0.3.0 is checksum-retained, uploaded with verified flash writes",
        "Bench B's later 0.2.0 LED-status candidate remains\n  compile-only; the working Bench B 0.1.0 semantic publisher remains deployed",
        "turning the potentiometer on Bench B visibly moved\nthe onboard RGB color on Bench A",
    )
    for statement in required_milestone:
        intake.require(statement in milestone, f"physical milestone statement drift: {statement}")
    return {
        "bench_a": {
            "portable_core_evidence": "selected_0.3.0_uploaded",
            "physical_can_milestone": "selected_0.3.0_uploaded",
            "comparison": "documented_consistent",
        },
        "bench_b": {
            "portable_core_evidence": "selected_0.2.0_uploaded",
            "physical_can_milestone": "0.2.0_compile_only_0.1.0_deployed",
            "comparison": "conflict_unresolved",
        },
        "initial_failure": {
            "evidence_form": "retained_documentary_summary",
            "peer_frame": "absent",
            "transitions": ["ERROR_PASSIVE", "BUS_OFF", "BUS_RECOVERED"],
        },
        "recovered_window": {
            "evidence_form": "retained_documentary_summary",
            "transport_pongs": {"first": 118, "last": 135},
            "semantic_sequences": {"first": 148, "last": 183},
            "quality": "GOOD",
            "values_approximately": "399..402",
            "stable_counters": {"bus_error_alerts": 245874, "bus_error_total": 263668},
        },
        "operator_observation": {
            "evidence_form": "retained_documentary_summary",
            "potentiometer_to_remote_rgb_response": "documented_working",
            "calibration": "not_assessed",
        },
    }


def verify_candidate_sources(candidate: dict[str, Any], expected: list[dict[str, str]]) -> None:
    manifest = intake.load_json(Path(candidate["manifest_path"]))
    sources = {item["path"]: item["sha256"] for item in manifest["source_inputs"]}
    for item in expected:
        intake.require(sources.get(item["path"]) == item["sha256"], f"candidate {candidate['role']} does not contain exact shared source {item['path']}")


def verify_retained_candidate(esp_root: Path, expected: dict[str, Any]) -> dict[str, Any]:
    manifest_path = intake.canonical_child(esp_root, expected["manifest_path"], f"candidate {expected['role']}")
    intake.require(intake.sha256_path(manifest_path) == expected["manifest_sha256"], f"candidate manifest digest mismatch for {expected['role']}")
    manifest = intake.load_json(manifest_path)
    intake.require(manifest.get("manifest_version") == 2, "candidate manifest version must be 2")
    for key in ("role", "firmware_version", "source_set_sha256", "expected_usb_serial"):
        intake.require(manifest.get(key) == expected[key], f"candidate {expected['role']} {key} drift")
    intake.require(manifest.get("candidate_status") == "compile_only_not_uploaded_not_physically_accepted", "retained candidate manifest improperly claims later evidence")
    intake.require(manifest.get("known_good") is False and manifest.get("safety", {}).get("upload_status") == "not_uploaded", "retained candidate manifest improperly claims realization")
    source_inputs = manifest.get("source_inputs")
    intake.require(isinstance(source_inputs, list) and source_inputs, "candidate source_inputs must not be empty")
    intake.require(manifest["source_set_sha256"] == intake.source_set_sha256(source_inputs), "candidate source-set receipt is internally inconsistent")
    artifact_receipts: list[dict[str, Any]] = []
    for artifact in manifest.get("artifacts", []):
        artifact_path = intake.canonical_child(manifest_path.parent, artifact["name"], f"candidate artifact {artifact['name']}")
        intake.require(artifact_path.stat().st_size == artifact["bytes"], f"candidate artifact size drift: {artifact['name']}")
        intake.require(intake.sha256_path(artifact_path) == artifact["sha256"], f"candidate artifact digest drift: {artifact['name']}")
        artifact_receipts.append({"name": artifact["name"], "bytes": artifact["bytes"], "sha256": artifact["sha256"]})
    intake.require(bool(artifact_receipts), "candidate artifacts must not be empty")
    return {
        **expected,
        "manifest_contract_path": expected["manifest_path"],
        "manifest_path": str(manifest_path),
        "candidate_status": manifest["candidate_status"],
        "artifacts": artifact_receipts,
    }


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
    intake.require(contract_path == DEFAULT_CONTRACT.resolve(), "the runner accepts only the fixed Slice F contract")
    intake.require(build_root == DEFAULT_BUILD_ROOT.resolve(), "the runner accepts only the fixed Slice F build root")
    intake.require(compiler.is_file() and os.access(compiler, os.X_OK), f"compiler is not executable: {compiler}")
    intake.require((esp_root / ".git").exists(), f"ESP_WORK_ROOT is not a Git checkout: {esp_root}")
    intake.require(Path(intake.git(esp_root, "rev-parse", "--show-toplevel", label="ESP Git root").strip()).resolve() == esp_root, "ESP_WORK_ROOT is not the exact Git top level")
    before_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status before Slice F")
    before_fingerprint, before_entry_count = intake.worktree_fingerprint(esp_root)

    contract = load_realization_contract(contract_path)
    slice_e_contract = intake.canonical_child(FISICS_ROOT, contract["slice_e"]["contract_path"], "Slice E contract")
    intake.require(intake.sha256_path(slice_e_contract) == contract["slice_e"]["contract_sha256"], "Slice E contract digest drift")
    slice_e_receipt_path = intake.canonical_child(FISICS_ROOT, contract["slice_e"]["receipt_path"], "Slice E receipt")
    slice_e_receipt = intake.load_json(slice_e_receipt_path)
    intake.require(slice_e_receipt.get("status") == "pass", "Slice E prerequisite did not pass")
    intake.require(slice_e_receipt.get("contract", {}).get("sha256") == contract["slice_e"]["contract_sha256"], "Slice E receipt contract linkage drift")
    expected_static = contract["static_artifacts"]
    actual_static = {
        "translation_unit_metadata_sha256": intake.sha256_path(build_root / "normalized_signal_policy.semantic.json"),
        "semantic_project_graph_sha256": intake.sha256_path(build_root / "e016.semantic-project.graph.json"),
        "physical_composition_graph_sha256": intake.sha256_path(build_root / "e016.physical-project.graph.json"),
        "hardware_compatibility_graph_sha256": intake.sha256_path(build_root / "e016.hardware-project.graph.json"),
    }
    intake.require(actual_static == expected_static, "Slice F static artifact projection drift")

    evidence_paths: dict[str, Path] = {}
    for item in contract["external_evidence"]:
        path = intake.canonical_child(esp_root, item["path"], f"retained evidence {item['role']}")
        intake.require(intake.sha256_path(path) == item["sha256"], f"retained evidence digest mismatch for {item['role']}")
        evidence_paths[item["role"]] = path

    candidates: dict[str, dict[str, Any]] = {}
    for expected in contract["candidates"]:
        candidates[expected["role"]] = verify_retained_candidate(esp_root, expected)
        verify_candidate_sources(candidates[expected["role"]], contract["shared_source"])

    current_source_projection = []
    for item in contract["shared_source"]:
        path = intake.canonical_child(esp_root, item["path"], f"current shared source {item['path']}")
        actual = intake.sha256_path(path)
        current_source_projection.append({
            "path": item["path"], "retained_sha256": item["sha256"],
            "current_sha256": actual, "comparison": "exact_match" if actual == item["sha256"] else "drift",
        })
    current_protocol = next(item for item in current_source_projection if item["path"] == "protocol/schema/protocol_v1.yaml")
    intake.require(current_protocol["comparison"] == "drift", "Slice F expected current protocol drift was not present")

    projections = verify_evidence_text(
        evidence_paths["portable_core_evidence"].read_text(encoding="utf-8"),
        evidence_paths["physical_can_milestone"].read_text(encoding="utf-8"),
        candidates,
    )
    intake.require(projections["bench_a"]["comparison"] == contract["expected_projection"]["bench_a_selected_candidate_upload"], "Bench A realization comparison drift")
    intake.require(projections["bench_b"]["comparison"] == contract["expected_projection"]["bench_b_selected_candidate_upload"], "Bench B realization comparison drift")

    after_fingerprint, after_entry_count = intake.worktree_fingerprint(esp_root)
    after_status = intake.git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status after Slice F")
    intake.require(after_fingerprint == before_fingerprint, "ESP worktree bytes changed during Slice F")
    intake.require(after_entry_count == before_entry_count, "ESP worktree entry count changed during Slice F")
    intake.require(after_status == before_status, "ESP Git status changed during Slice F")

    receipt = {
        "schema": "fisiCs.esp_realization_evidence_receipt",
        "schema_version": 1,
        "lane_id": contract["lane_id"],
        "status": "pass_with_reported_mismatch",
        "contract": {"path": str(contract_path), "sha256": intake.sha256_path(contract_path)},
        "slice_e": {"receipt_path": str(slice_e_receipt_path), "receipt_sha256": intake.sha256_path(slice_e_receipt_path), "status": "pass"},
        "static_projection": actual_static,
        "candidate_projection": {
            role: {
                "manifest_path": item["manifest_path"],
                "manifest_sha256": item["manifest_sha256"],
                "source_set_sha256": item["source_set_sha256"],
                "firmware_version": item["firmware_version"],
                "expected_usb_serial": item["expected_usb_serial"],
                "manifest_state": item["candidate_status"],
            }
            for role, item in sorted(candidates.items())
        },
        "current_source_projection": current_source_projection,
        "evidence_sources": [
            {"role": item["role"], "path": str(evidence_paths[item["role"]]), "sha256": item["sha256"], "form": "retained_documentary_summary"}
            for item in contract["external_evidence"]
        ],
        "realization_projection": projections,
        "comparison": contract["expected_projection"],
        "mismatches": [
            {
                "id": "bench_b_selected_candidate_deployment",
                "state": "conflict_unresolved",
                "left": "portable_core_evidence documents selected Bench B 0.2.0 upload",
                "right": "physical_can_milestone says Bench B 0.2.0 compile-only and 0.1.0 deployed",
                "effect": "exact selected-pair deployment and runtime linkage is not established across retained evidence",
                "repair": "none_performed",
            },
            {
                "id": "current_protocol_vs_retained_projection",
                "state": "drift_reported",
                "retained_sha256": current_protocol["retained_sha256"],
                "current_sha256": current_protocol["current_sha256"],
                "effect": "the retained Slice E graph remains historical evidence but cannot be regenerated from the current ESP protocol bytes",
                "repair": "none_performed",
            }
        ],
        "evidence_gaps": [
            {
                "id": "exact_bench_b_deployment_receipt",
                "state": "not_retained_in_selected_evidence",
                "effect": "candidate, target USB identity, upload result, and flash verification are not joined by one immutable structured receipt",
                "bench_rerun_required_for_compiler": False,
            },
            {
                "id": "raw_runtime_capture",
                "state": "not_retained_in_selected_evidence",
                "effect": "documentary runtime summaries cannot be validated against exact captured bytes",
                "bench_rerun_required_for_compiler": False,
            },
        ],
        "claim_boundary": {
            "static_graph_chain": "established",
            "candidate_integrity": "validated",
            "shared_source_candidate_linkage": "exact_match",
            "current_protocol_vs_retained_projection": "drift_reported",
            "bench_a_selected_candidate_upload": "retained_documentary_evidence_consistent",
            "bench_b_selected_candidate_upload": "conflict_unresolved",
            "retained_runtime_observation": "documented_summary",
            "retained_operator_physical_observation": "documented_summary",
            "structured_deployment_receipt": "not_retained_in_selected_evidence",
            "structured_observation_receipt": "not_retained_in_selected_evidence",
            "raw_serial_capture": "not_retained_in_selected_evidence",
            "exact_selected_pair_realization": "not_established",
            "fresh_runtime_observation": "not_assessed",
            "current_hardware_state": "not_assessed",
            "calibration": "not_assessed",
            "deployment_or_release_action": "not_performed",
        },
        "external_repository": {
            "path": str(esp_root),
            "head": intake.git(esp_root, "rev-parse", "HEAD", label="ESP HEAD").strip(),
            "status_sha256": intake.sha256_bytes(before_status.encode()),
            "worktree_entry_count": before_entry_count,
            "worktree_fingerprint_sha256": before_fingerprint,
            "unchanged_during_slice_f": True,
        },
    }
    first = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode()
    second = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode()
    intake.require(first == second, "Slice F receipt serialization is not deterministic")
    intake.atomic_write(build_root / "e016.realization-evidence.receipt.json", first)
    print("E016 Slice F receipt=PASS mismatches=2 exact_selected_pair=NOT_ESTABLISHED raw_serial=NOT_RETAINED external_tree=UNCHANGED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"E016 Slice F: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
