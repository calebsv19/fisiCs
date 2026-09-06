#!/usr/bin/env python3
"""Validate digest-bound deployment and raw observation receipt chains."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path, PurePosixPath
from typing import Any


DEPLOYMENT_SCHEMA = "fisiCs.external_deployment_receipt"
OBSERVATION_SCHEMA = "fisiCs.external_observation_receipt"
CHAIN_SCHEMA = "fisiCs.external_evidence_chain_receipt"
EVIDENCE_CLASSES = {"contract_fixture", "retained_external"}
SHA256_RE = re.compile(r"[0-9a-f]{64}")
USB_SERIAL_RE = re.compile(r"(?:[0-9A-F]{2}:){5}[0-9A-F]{2}")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def exact_keys(value: Any, keys: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    actual = set(value)
    require(actual == keys, f"{label} fields mismatch: missing={sorted(keys - actual)} extra={sorted(actual - keys)}")
    return value


def nonempty(value: Any, label: str) -> str:
    require(isinstance(value, str) and bool(value.strip()), f"{label} must be a non-empty string")
    return value


def digest(value: Any, label: str) -> str:
    text = nonempty(value, label)
    require(SHA256_RE.fullmatch(text) is not None, f"{label} must be a lowercase SHA-256")
    return text


def relative_path(value: Any, label: str) -> str:
    text = nonempty(value, label)
    path = PurePosixPath(text)
    require(
        not path.is_absolute()
        and ".." not in path.parts
        and "." not in path.parts
        and str(path) == text,
        f"{label} must be a normalized relative path",
    )
    return text


def child(root: Path, value: Any, label: str) -> Path:
    text = relative_path(value, label)
    resolved = (root / text).resolve()
    require(resolved == root or root in resolved.parents, f"{label} escapes the evidence root")
    require(resolved.is_file(), f"{label} is not a file: {text}")
    return resolved


def load_json(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"cannot read {label}: {exc}") from exc
    require(isinstance(value, dict), f"{label} must contain a JSON object")
    return value


def sha256(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def verify_file(
    root: Path,
    record: dict[str, Any],
    label: str,
    *,
    require_bytes: bool = False,
    extra_keys: set[str] | None = None,
) -> Path:
    keys = {"path", "sha256", "bytes"} if require_bytes else {"path", "sha256"}
    exact_keys(record, keys | (extra_keys or set()), label)
    path = child(root, record["path"], f"{label}.path")
    require(sha256(path) == digest(record["sha256"], f"{label}.sha256"), f"{label} digest mismatch")
    if require_bytes:
        require(isinstance(record["bytes"], int) and record["bytes"] >= 0, f"{label}.bytes must be non-negative")
        require(path.stat().st_size == record["bytes"], f"{label} byte count mismatch")
    return path


def parse_utc(value: Any, label: str) -> datetime:
    text = nonempty(value, label)
    require(text.endswith("Z"), f"{label} must be an explicit UTC timestamp ending in Z")
    try:
        result = datetime.fromisoformat(text[:-1] + "+00:00")
    except ValueError as exc:
        raise RuntimeError(f"{label} is not ISO-8601") from exc
    require(result.tzinfo == timezone.utc, f"{label} must be UTC")
    return result


def validate_window(record: dict[str, Any], label: str) -> None:
    started = parse_utc(record["started_at_utc"], f"{label}.started_at_utc")
    completed = parse_utc(record["completed_at_utc"], f"{label}.completed_at_utc")
    require(completed >= started, f"{label} completed before it started")


def validate_deployment(root: Path, receipt_path: Path) -> dict[str, Any]:
    root = root.resolve()
    receipt_path = receipt_path.resolve()
    require(root in receipt_path.parents and receipt_path.is_file(), "deployment receipt must be a file inside the evidence root")
    data = exact_keys(
        load_json(receipt_path, "deployment receipt"),
        {"schema", "schema_version", "receipt_id", "evidence_class", "candidate", "target", "operation", "authority", "claim_boundary"},
        "deployment receipt",
    )
    require(data["schema"] == DEPLOYMENT_SCHEMA and data["schema_version"] == 1, "unsupported deployment receipt schema")
    receipt_id = nonempty(data["receipt_id"], "deployment receipt_id")
    evidence_class = nonempty(data["evidence_class"], "deployment evidence_class")
    require(evidence_class in EVIDENCE_CLASSES, "unsupported deployment evidence_class")

    candidate = exact_keys(data["candidate"], {"manifest", "artifact", "source_set_sha256", "protocol"}, "deployment candidate")
    manifest_path = verify_file(root, candidate["manifest"], "deployment candidate manifest")
    artifact_path = verify_file(root, candidate["artifact"], "deployment candidate artifact", require_bytes=True)
    protocol_path = verify_file(root, candidate["protocol"], "deployment candidate protocol")
    source_set = digest(candidate["source_set_sha256"], "deployment candidate source_set_sha256")

    manifest = load_json(manifest_path, "candidate manifest")
    require(manifest.get("manifest_version") == 2, "candidate manifest_version must be 2")
    require(manifest.get("source_set_sha256") == source_set, "candidate source-set linkage mismatch")
    source_inputs = manifest.get("source_inputs")
    require(isinstance(source_inputs, list), "candidate source_inputs must be a list")
    protocol_digest = sha256(protocol_path)
    require(any(isinstance(item, dict) and item.get("sha256") == protocol_digest for item in source_inputs), "candidate manifest does not bind the protocol digest")
    artifacts = manifest.get("artifacts")
    require(isinstance(artifacts, list), "candidate artifacts must be a list")
    require(any(
        isinstance(item, dict)
        and item.get("name") == artifact_path.name
        and item.get("sha256") == sha256(artifact_path)
        and item.get("bytes") == artifact_path.stat().st_size
        for item in artifacts
    ), "candidate manifest does not bind the selected artifact")

    target = exact_keys(data["target"], {"board_profile", "usb_serial"}, "deployment target")
    nonempty(target["board_profile"], "deployment target.board_profile")
    require(USB_SERIAL_RE.fullmatch(nonempty(target["usb_serial"], "deployment target.usb_serial")) is not None, "deployment target.usb_serial must be an uppercase six-byte identity")

    operation = exact_keys(data["operation"], {"result", "tool", "started_at_utc", "completed_at_utc", "identity_check", "flash_verification"}, "deployment operation")
    require(operation["result"] in {"pass", "failed"}, "unsupported deployment operation result")
    require(operation["identity_check"] in {"matched", "failed"}, "unsupported identity_check")
    require(operation["flash_verification"] in {"passed", "failed", "not_run"}, "unsupported flash_verification")
    nonempty(operation["tool"], "deployment operation.tool")
    validate_window(operation, "deployment operation")
    if operation["result"] == "pass":
        require(operation["identity_check"] == "matched" and operation["flash_verification"] == "passed", "passing deployment requires matched identity and passed flash verification")

    authority = exact_keys(data["authority"], {"kind", "reference"}, "deployment authority")
    require(authority["kind"] in {"contract_fixture", "operator_authorized", "automated_authorized"}, "unsupported deployment authority kind")
    nonempty(authority["reference"], "deployment authority.reference")
    require((evidence_class == "contract_fixture") == (authority["kind"] == "contract_fixture"), "fixture authority must match fixture evidence_class")

    claims = exact_keys(data["claim_boundary"], {"candidate_integrity", "deployment", "runtime_observation", "physical_acceptance"}, "deployment claim_boundary")
    require(claims["candidate_integrity"] == "digest_validated", "candidate_integrity must be digest_validated")
    expected_deployment = "fixture_only" if evidence_class == "contract_fixture" else ("established" if operation["result"] == "pass" else "failed")
    require(claims["deployment"] == expected_deployment, "deployment claim does not match evidence class and operation result")
    require(claims["runtime_observation"] == "not_assessed" and claims["physical_acceptance"] == "not_assessed", "deployment receipt may not promote runtime or physical claims")

    return {
        "receipt_id": receipt_id,
        "receipt_path": str(receipt_path.relative_to(root)),
        "receipt_sha256": sha256(receipt_path),
        "evidence_class": evidence_class,
        "candidate_manifest_sha256": sha256(manifest_path),
        "artifact_sha256": sha256(artifact_path),
        "protocol_sha256": protocol_digest,
        "source_set_sha256": source_set,
        "usb_serial": target["usb_serial"],
        "deployment": claims["deployment"],
        "completed_at_utc": operation["completed_at_utc"],
    }


def validate_observation(root: Path, receipt_path: Path) -> dict[str, Any]:
    root = root.resolve()
    receipt_path = receipt_path.resolve()
    require(root in receipt_path.parents and receipt_path.is_file(), "observation receipt must be a file inside the evidence root")
    data = exact_keys(
        load_json(receipt_path, "observation receipt"),
        {"schema", "schema_version", "receipt_id", "evidence_class", "deployment_receipts", "capture", "assertions", "operator_observation", "claim_boundary"},
        "observation receipt",
    )
    require(data["schema"] == OBSERVATION_SCHEMA and data["schema_version"] == 1, "unsupported observation receipt schema")
    receipt_id = nonempty(data["receipt_id"], "observation receipt_id")
    evidence_class = nonempty(data["evidence_class"], "observation evidence_class")
    require(evidence_class in EVIDENCE_CLASSES, "unsupported observation evidence_class")

    references = data["deployment_receipts"]
    require(isinstance(references, list) and references, "observation deployment_receipts must be a non-empty list")
    deployments: list[dict[str, Any]] = []
    roles: set[str] = set()
    ids: set[str] = set()
    for index, reference in enumerate(references):
        label = f"observation deployment_receipts[{index}]"
        exact_keys(reference, {"role", "path", "sha256", "receipt_id"}, label)
        role = nonempty(reference["role"], f"{label}.role")
        require(role not in roles, "duplicate observation deployment role")
        roles.add(role)
        path = child(root, reference["path"], f"{label}.path")
        require(sha256(path) == digest(reference["sha256"], f"{label}.sha256"), f"{label} digest mismatch")
        validated = validate_deployment(root, path)
        require(validated["receipt_id"] == reference["receipt_id"], f"{label} receipt_id mismatch")
        require(validated["receipt_id"] not in ids, "duplicate observation deployment receipt_id")
        require(validated["evidence_class"] == evidence_class, "deployment and observation evidence_class mismatch")
        ids.add(validated["receipt_id"])
        deployments.append({"role": role, **validated})

    capture = exact_keys(data["capture"], {"kind", "path", "sha256", "bytes", "started_at_utc", "completed_at_utc"}, "observation capture")
    require(capture["kind"] in {"serial_text", "can_trace_text"}, "unsupported observation capture kind")
    capture_path = verify_file(
        root,
        capture,
        "observation capture",
        require_bytes=True,
        extra_keys={"kind", "started_at_utc", "completed_at_utc"},
    )
    validate_window(capture, "observation capture")
    capture_started = parse_utc(capture["started_at_utc"], "observation capture.started_at_utc")
    require(all(capture_started >= parse_utc(item["completed_at_utc"], "deployment completed_at_utc") for item in deployments), "observation capture started before a referenced deployment completed")
    if evidence_class == "retained_external":
        require(all(item["deployment"] == "established" for item in deployments), "retained observation requires established deployment receipts")
    try:
        capture_text = capture_path.read_text(encoding="utf-8")
    except UnicodeDecodeError as exc:
        raise RuntimeError("observation capture must be UTF-8 text in schema v1") from exc

    assertions = data["assertions"]
    require(isinstance(assertions, list) and assertions, "observation assertions must be a non-empty list")
    assertion_ids: set[str] = set()
    validated_assertions: list[dict[str, str]] = []
    for index, assertion in enumerate(assertions):
        label = f"observation assertions[{index}]"
        exact_keys(assertion, {"id", "kind", "contains", "summary"}, label)
        assertion_id = nonempty(assertion["id"], f"{label}.id")
        require(assertion_id not in assertion_ids, "duplicate observation assertion id")
        assertion_ids.add(assertion_id)
        kind = nonempty(assertion["kind"], f"{label}.kind")
        contains = nonempty(assertion["contains"], f"{label}.contains")
        require("\n" not in contains and "\r" not in contains, f"{label}.contains must be one exact line fragment")
        require(contains in capture_text, f"observation assertion not found in raw capture: {assertion_id}")
        validated_assertions.append({"id": assertion_id, "kind": kind, "summary": nonempty(assertion["summary"], f"{label}.summary")})

    operator = exact_keys(data["operator_observation"], {"state", "summary"}, "operator_observation")
    require(operator["state"] in {"not_assessed", "documented_external"}, "unsupported operator observation state")
    nonempty(operator["summary"], "operator_observation.summary")
    require(not (evidence_class == "contract_fixture" and operator["state"] != "not_assessed"), "contract fixture may not claim an operator observation")

    claims = exact_keys(data["claim_boundary"], {"deployment_linkage", "raw_capture_integrity", "digital_runtime_observation", "physical_acceptance", "calibration"}, "observation claim_boundary")
    if evidence_class == "contract_fixture":
        require(claims["deployment_linkage"] == "fixture_only" and claims["raw_capture_integrity"] == "fixture_only" and claims["digital_runtime_observation"] == "fixture_only", "contract fixture claims must remain fixture_only")
    else:
        require(claims["deployment_linkage"] == "digest_validated" and claims["raw_capture_integrity"] == "digest_validated" and claims["digital_runtime_observation"] == "raw_capture_bound", "retained external claims must use digest-bound vocabulary")
    expected_physical = "documented_external" if operator["state"] == "documented_external" else "not_assessed"
    require(claims["physical_acceptance"] == expected_physical, "physical acceptance claim does not match operator observation")
    require(claims["calibration"] == "not_assessed", "observation receipt v1 may not establish calibration")

    return {
        "schema": CHAIN_SCHEMA,
        "schema_version": 1,
        "status": "pass",
        "evidence_class": evidence_class,
        "deployments": sorted(deployments, key=lambda item: item["role"]),
        "observation": {
            "receipt_id": receipt_id,
            "receipt_path": str(receipt_path.relative_to(root)),
            "receipt_sha256": sha256(receipt_path),
            "capture_path": str(capture_path.relative_to(root)),
            "capture_sha256": sha256(capture_path),
            "capture_bytes": capture_path.stat().st_size,
            "assertions": validated_assertions,
            "operator_observation": operator["state"],
        },
        "claim_boundary": claims,
    }


def canonical_bytes(value: dict[str, Any]) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "wb") as handle:
            handle.write(data)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--evidence-root", required=True, type=Path)
    parser.add_argument("--observation", required=True, type=Path, help="observation receipt path relative to the evidence root")
    parser.add_argument("--output", type=Path, help="optional deterministic chain-receipt output")
    args = parser.parse_args()

    root = args.evidence_root.resolve()
    require(root.is_dir(), "evidence root must be a directory")
    observation_path = child(root, str(args.observation), "observation argument")
    result = validate_observation(root, observation_path)
    output = canonical_bytes(result)
    if args.output:
        atomic_write(args.output.resolve(), output)
    else:
        print(output.decode("utf-8"), end="")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"external evidence validation: FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
