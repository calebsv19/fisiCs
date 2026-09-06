#!/usr/bin/env python3
"""Validate one digest-bound ESP-owned portable C intake without ESP writes."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Any


FISICS_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONTRACT = (
    FISICS_ROOT / "tests/fixtures/esp_portable_c/e016_intake_v1.json"
)
SHA256_RE = re.compile(r"^[0-9a-f]{64}$")
IDENTITY_RE = re.compile(r"^[a-z][a-z0-9_]*(?:\.[a-z][a-z0-9_]*)*$")


def fail(message: str) -> None:
    raise RuntimeError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def strict_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"), object_pairs_hook=strict_object)
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        fail(f"cannot read JSON {path}: {exc}")


def exact_keys(value: Any, keys: set[str], label: str) -> dict[str, Any]:
    require(isinstance(value, dict), f"{label} must be an object")
    actual = set(value)
    require(actual == keys, f"{label} fields mismatch: expected={sorted(keys)} actual={sorted(actual)}")
    return value


def nonempty_string(value: Any, label: str) -> str:
    require(isinstance(value, str) and bool(value), f"{label} must be a non-empty string")
    return value


def digest_string(value: Any, label: str) -> str:
    value = nonempty_string(value, label)
    require(bool(SHA256_RE.fullmatch(value)), f"{label} must be lowercase SHA-256")
    return value


def relative_path(value: Any, label: str) -> str:
    text = nonempty_string(value, label)
    path = Path(text)
    require(not path.is_absolute() and ".." not in path.parts, f"{label} must be repository-relative without '..'")
    require(path.as_posix() == text, f"{label} must use canonical POSIX spelling")
    return text


def canonical_child(root: Path, relative: str, label: str) -> Path:
    root = root.resolve()
    resolved = (root / relative).resolve()
    require(resolved != root and root in resolved.parents, f"{label} escapes repository root")
    require(resolved.is_file(), f"{label} is unavailable: {resolved}")
    return resolved


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def source_set_sha256(entries: list[dict[str, Any]]) -> str:
    encoded = json.dumps(entries, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return sha256_bytes(encoded)


def worktree_fingerprint(root: Path) -> tuple[str, int]:
    """Hash every non-Git worktree entry without following directory symlinks."""
    entries: list[dict[str, Any]] = []
    for directory, directory_names, file_names in os.walk(root, followlinks=False):
        directory_names[:] = sorted(name for name in directory_names if name != ".git")
        base = Path(directory)
        for name in sorted(file_names):
            path = base / name
            relative = path.relative_to(root).as_posix()
            if path.is_symlink():
                entries.append({"path": relative, "kind": "symlink", "target": os.readlink(path)})
            elif path.is_file():
                entries.append({
                    "path": relative,
                    "kind": "file",
                    "bytes": path.stat().st_size,
                    "sha256": sha256_path(path),
                })
            else:
                fail(f"unsupported worktree entry while fingerprinting: {path}")
    encoded = json.dumps(entries, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return sha256_bytes(encoded), len(entries)


def run(
    command: list[str],
    *,
    cwd: Path,
    label: str,
    env: dict[str, str] | None = None,
) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        fail(
            f"{label} failed with exit {completed.returncode}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    return completed


def git(root: Path, *arguments: str, label: str) -> str:
    return run(["git", *arguments], cwd=root, label=label).stdout


def load_contract(path: Path) -> dict[str, Any]:
    data = exact_keys(
        load_json(path),
        {
            "schema", "schema_version", "lane_id", "external_inputs",
            "compiler_header", "behavior", "protocol", "metadata", "candidates",
        },
        "intake contract",
    )
    require(data["schema"] == "fisiCs.esp_portable_c_intake", "unsupported intake schema")
    require(data["schema_version"] == 1, "unsupported intake schema version")
    nonempty_string(data["lane_id"], "lane_id")

    inputs = data["external_inputs"]
    require(isinstance(inputs, list) and inputs, "external_inputs must select at least one input")
    roles: set[str] = set()
    paths: set[str] = set()
    for index, item in enumerate(inputs):
        item = exact_keys(item, {"role", "path", "sha256"}, f"external_inputs[{index}]")
        role = nonempty_string(item["role"], f"external_inputs[{index}].role")
        path_value = relative_path(item["path"], f"external_inputs[{index}].path")
        digest_string(item["sha256"], f"external_inputs[{index}].sha256")
        require(role not in roles, f"duplicate external input role: {role}")
        require(path_value not in paths, f"duplicate external input path: {path_value}")
        roles.add(role)
        paths.add(path_value)

    header = exact_keys(data["compiler_header"], {"path", "sha256"}, "compiler_header")
    relative_path(header["path"], "compiler_header.path")
    digest_string(header["sha256"], "compiler_header.sha256")

    behavior = exact_keys(
        data["behavior"],
        {
            "source_role", "test_role", "include_path", "normalization_cases",
            "publication_cases", "runs_per_compiler", "expected_stdout",
        },
        "behavior",
    )
    for key in ("source_role", "test_role"):
        role = nonempty_string(behavior[key], f"behavior.{key}")
        require(role in roles, f"behavior.{key} references unknown role: {role}")
    relative_path(behavior["include_path"], "behavior.include_path")
    frozen_counts = {
        "normalization_cases": 6,
        "publication_cases": 12,
        "runs_per_compiler": 2,
    }
    for key, expected_count in frozen_counts.items():
        require(
            isinstance(behavior[key], int)
            and not isinstance(behavior[key], bool)
            and behavior[key] == expected_count,
            f"behavior.{key} must equal the frozen value {expected_count}",
        )
    expected = nonempty_string(behavior["expected_stdout"], "behavior.expected_stdout")
    frozen_stdout = (
        "portable_normalized_signal_policy_v1 normalization=6 "
        "publication=12 PASS\n"
    )
    require(expected == frozen_stdout, "behavior.expected_stdout differs from the frozen oracle")

    protocol = exact_keys(
        data["protocol"],
        {
            "mapping_role", "mapping_schema", "protocol_schema_role",
            "canonical_protocol_sha256", "semantic_type", "instance", "target_identity",
        },
        "protocol",
    )
    for key in ("mapping_role", "protocol_schema_role"):
        role = nonempty_string(protocol[key], f"protocol.{key}")
        require(role in roles, f"protocol.{key} references unknown role: {role}")
    nonempty_string(protocol["mapping_schema"], "protocol.mapping_schema")
    digest_string(protocol["canonical_protocol_sha256"], "protocol.canonical_protocol_sha256")
    nonempty_string(protocol["semantic_type"], "protocol.semantic_type")
    nonempty_string(protocol["instance"], "protocol.instance")
    target = nonempty_string(protocol["target_identity"], "protocol.target_identity")
    require(bool(IDENTITY_RE.fullmatch(target)), "protocol.target_identity is not canonical")

    metadata = exact_keys(
        data["metadata"],
        {
            "schema", "schema_version", "analysis_contract_id",
            "analysis_contract_minimum", "records", "edges",
        },
        "metadata",
    )
    require(metadata["schema"] == "fisiCs.semantic_metadata", "metadata schema must be fisiCs.semantic_metadata")
    require(metadata["schema_version"] == 1, "metadata schema version must be 1")
    nonempty_string(metadata["analysis_contract_id"], "metadata.analysis_contract_id")
    nonempty_string(metadata["analysis_contract_minimum"], "metadata.analysis_contract_minimum")
    require(isinstance(metadata["records"], list) and metadata["records"], "metadata.records must not be empty")
    record_ids: set[str] = set()
    record_symbols: set[str] = set()
    for index, record in enumerate(metadata["records"]):
        record = exact_keys(record, {"symbol", "identity"}, f"metadata.records[{index}]")
        symbol = nonempty_string(record["symbol"], f"metadata.records[{index}].symbol")
        identity = nonempty_string(record["identity"], f"metadata.records[{index}].identity")
        require(bool(IDENTITY_RE.fullmatch(identity)), f"metadata.records[{index}].identity is not canonical")
        require(symbol not in record_symbols and identity not in record_ids, "duplicate expected metadata record")
        record_symbols.add(symbol)
        record_ids.add(identity)
    require(isinstance(metadata["edges"], list) and metadata["edges"], "metadata.edges must not be empty")
    edge_keys: set[tuple[str, str, str]] = set()
    for index, edge in enumerate(metadata["edges"]):
        edge = exact_keys(
            edge,
            {"source_identity", "kind", "target_identity", "resolution"},
            f"metadata.edges[{index}]",
        )
        source = nonempty_string(edge["source_identity"], f"metadata.edges[{index}].source_identity")
        require(source in record_ids, f"metadata edge references unknown source identity: {source}")
        require(edge["kind"] in ("produces", "consumes"), "metadata edge kind must be produces or consumes")
        require(edge["resolution"] == "external", "intake metadata edge resolution must be external")
        require(edge["target_identity"] == target, "metadata edge target must equal protocol target")
        key = (source, edge["kind"], edge["target_identity"])
        require(key not in edge_keys, "duplicate expected metadata edge")
        edge_keys.add(key)

    candidates = data["candidates"]
    require(isinstance(candidates, list) and candidates, "candidates must select at least one retained manifest")
    candidate_roles: set[str] = set()
    candidate_paths: set[str] = set()
    for index, candidate in enumerate(candidates):
        candidate = exact_keys(
            candidate,
            {
                "role", "manifest_path", "manifest_sha256", "source_set_sha256",
                "firmware_version", "expected_usb_serial",
            },
            f"candidates[{index}]",
        )
        role = nonempty_string(candidate["role"], f"candidates[{index}].role")
        manifest_path = relative_path(candidate["manifest_path"], f"candidates[{index}].manifest_path")
        digest_string(candidate["manifest_sha256"], f"candidates[{index}].manifest_sha256")
        digest_string(candidate["source_set_sha256"], f"candidates[{index}].source_set_sha256")
        nonempty_string(candidate["firmware_version"], f"candidates[{index}].firmware_version")
        serial = nonempty_string(candidate["expected_usb_serial"], f"candidates[{index}].expected_usb_serial")
        require(len(serial.split(":")) == 6, f"candidates[{index}] USB serial is malformed")
        require(role not in candidate_roles, f"duplicate candidate role: {role}")
        require(manifest_path not in candidate_paths, f"duplicate candidate manifest: {manifest_path}")
        candidate_roles.add(role)
        candidate_paths.add(manifest_path)
    return data


def input_map(contract: dict[str, Any]) -> dict[str, dict[str, str]]:
    return {item["role"]: item for item in contract["external_inputs"]}


def verify_external_inputs(root: Path, contract: dict[str, Any]) -> dict[str, str]:
    verified: dict[str, str] = {}
    for item in contract["external_inputs"]:
        path = canonical_child(root, item["path"], f"external input {item['role']}")
        actual = sha256_path(path)
        require(actual == item["sha256"], f"external input digest mismatch for {item['role']}: expected={item['sha256']} actual={actual}")
        verified[item["role"]] = actual
    return verified


def verify_protocol(root: Path, contract: dict[str, Any]) -> None:
    inputs = input_map(contract)
    spec = contract["protocol"]
    mapping_path = canonical_child(root, inputs[spec["mapping_role"]]["path"], "semantic mapping")
    mapping = exact_keys(
        load_json(mapping_path),
        {"schema", "protocol_schema", "protocol_schema_sha256", "mappings"},
        "semantic mapping",
    )
    require(mapping["schema"] == spec["mapping_schema"], "semantic mapping schema drift")
    require(mapping["protocol_schema"] == inputs[spec["protocol_schema_role"]]["path"], "semantic mapping protocol path drift")
    require(mapping["protocol_schema_sha256"] == spec["canonical_protocol_sha256"], "canonical protocol digest drift")
    require(isinstance(mapping["mappings"], list) and len(mapping["mappings"]) == 1, "semantic mapping must contain exactly one mapping")
    entry = exact_keys(
        mapping["mappings"][0],
        {"semantic_type", "instance", "canonical_semantic_identity", "source_operations"},
        "semantic mapping entry",
    )
    require(entry["semantic_type"] == spec["semantic_type"], "semantic type drift")
    require(entry["instance"] == spec["instance"], "semantic instance drift")
    require(entry["canonical_semantic_identity"] == spec["target_identity"], "protocol target identity drift")
    expected = {
        (record["symbol"], record["identity"], edge["kind"])
        for record in contract["metadata"]["records"]
        for edge in contract["metadata"]["edges"]
        if edge["source_identity"] == record["identity"]
    }
    actual: set[tuple[str, str, str]] = set()
    require(isinstance(entry["source_operations"], list), "source_operations must be an array")
    for index, operation in enumerate(entry["source_operations"]):
        operation = exact_keys(
            operation,
            {"symbol", "canonical_semantic_identity", "relationship"},
            f"source_operations[{index}]",
        )
        actual.add((operation["symbol"], operation["canonical_semantic_identity"], operation["relationship"]))
    require(actual == expected, f"protocol source-operation mapping drift: expected={sorted(expected)} actual={sorted(actual)}")


def verify_manifest(root: Path, expected: dict[str, Any]) -> dict[str, Any]:
    manifest_path = canonical_child(root, expected["manifest_path"], f"candidate {expected['role']}")
    actual_manifest_sha = sha256_path(manifest_path)
    require(actual_manifest_sha == expected["manifest_sha256"], f"candidate manifest digest mismatch for {expected['role']}")
    manifest = load_json(manifest_path)
    require(isinstance(manifest, dict), "candidate manifest must be an object")
    require(manifest.get("manifest_version") == 2, "candidate manifest version must be 2")
    require(manifest.get("role") == expected["role"], "candidate role drift")
    require(manifest.get("firmware_version") == expected["firmware_version"], "candidate firmware version drift")
    require(manifest.get("expected_usb_serial", "").upper() == expected["expected_usb_serial"].upper(), "candidate USB identity drift")
    require(manifest.get("source_set_sha256") == expected["source_set_sha256"], "candidate source-set digest drift")
    require(manifest.get("candidate_status") == "compile_only_not_uploaded_not_physically_accepted", "candidate improperly claims later evidence")
    require(manifest.get("known_good") is False, "candidate improperly claims known-good")
    source_inputs = manifest.get("source_inputs")
    require(isinstance(source_inputs, list) and source_inputs, "candidate source_inputs must not be empty")
    require(manifest["source_set_sha256"] == source_set_sha256(source_inputs), "candidate source-set receipt is internally inconsistent")
    paths: list[str] = []
    for index, source in enumerate(source_inputs):
        source = exact_keys(source, {"path", "bytes", "sha256"}, f"candidate source_inputs[{index}]")
        relative = relative_path(source["path"], f"candidate source_inputs[{index}].path")
        require(isinstance(source["bytes"], int) and not isinstance(source["bytes"], bool) and source["bytes"] >= 0, "candidate source byte count is invalid")
        digest_string(source["sha256"], f"candidate source_inputs[{index}].sha256")
        source_path = canonical_child(root, relative, f"candidate source input {relative}")
        require(source_path.stat().st_size == source["bytes"], f"candidate source size drift: {relative}")
        require(sha256_path(source_path) == source["sha256"], f"candidate source digest drift: {relative}")
        paths.append(relative)
    require(paths == sorted(paths) and len(paths) == len(set(paths)), "candidate source paths must be unique and sorted")
    artifacts = manifest.get("artifacts")
    require(isinstance(artifacts, list) and artifacts, "candidate artifacts must not be empty")
    artifact_receipts: list[dict[str, Any]] = []
    for index, artifact in enumerate(artifacts):
        artifact = exact_keys(artifact, {"name", "bytes", "sha256"}, f"candidate artifacts[{index}]")
        name = relative_path(artifact["name"], f"candidate artifacts[{index}].name")
        require(len(Path(name).parts) == 1, "candidate artifact name must not contain directories")
        require(isinstance(artifact["bytes"], int) and not isinstance(artifact["bytes"], bool) and artifact["bytes"] >= 0, "candidate artifact byte count is invalid")
        digest_string(artifact["sha256"], f"candidate artifacts[{index}].sha256")
        artifact_path = canonical_child(manifest_path.parent, name, f"candidate artifact {name}")
        require(artifact_path.stat().st_size == artifact["bytes"], f"candidate artifact size drift: {name}")
        require(sha256_path(artifact_path) == artifact["sha256"], f"candidate artifact digest drift: {name}")
        artifact_receipts.append({"name": name, "bytes": artifact["bytes"], "sha256": artifact["sha256"]})
    return {
        "role": expected["role"],
        "manifest_path": expected["manifest_path"],
        "manifest_sha256": actual_manifest_sha,
        "source_set_sha256": manifest["source_set_sha256"],
        "firmware_version": manifest["firmware_version"],
        "expected_usb_serial": manifest["expected_usb_serial"],
        "candidate_status": manifest["candidate_status"],
        "artifacts": artifact_receipts,
    }


def version_tuple(value: str, label: str) -> tuple[int, int, int]:
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", value)
    require(match is not None, f"{label} must be major.minor.patch")
    return tuple(int(part) for part in match.groups())  # type: ignore[return-value]


def verify_metadata(path: Path, source: Path, contract: dict[str, Any]) -> dict[str, Any]:
    document = exact_keys(
        load_json(path),
        {"schema", "schema_version", "producer", "analysis_contract", "source", "complete", "records", "edges"},
        "semantic metadata artifact",
    )
    spec = contract["metadata"]
    require(document["schema"] == spec["schema"] and document["schema_version"] == spec["schema_version"], "semantic metadata schema drift")
    require(document["complete"] is True, "semantic metadata artifact is incomplete")
    producer = exact_keys(document["producer"], {"name", "version"}, "semantic metadata producer")
    require(producer["name"] == "fisiCs", "semantic metadata producer drift")
    analysis = exact_keys(document["analysis_contract"], {"id", "version"}, "semantic metadata analysis contract")
    require(analysis["id"] == spec["analysis_contract_id"], "analysis contract ID drift")
    require(version_tuple(analysis["version"], "analysis contract version") >= version_tuple(spec["analysis_contract_minimum"], "minimum analysis contract version"), "analysis contract version is too old")
    source_receipt = exact_keys(document["source"], {"path", "hash_fnv1a64", "length_bytes"}, "semantic metadata source receipt")
    require(Path(source_receipt["path"]).resolve() == source.resolve(), "semantic metadata source path drift")
    require(source_receipt["length_bytes"] == source.stat().st_size, "semantic metadata source length drift")
    require(isinstance(source_receipt["hash_fnv1a64"], str) and bool(re.fullmatch(r"[0-9a-f]{16}", source_receipt["hash_fnv1a64"])), "semantic metadata source hash is malformed")
    expected_records = [(item["symbol"], item["identity"]) for item in spec["records"]]
    actual_records = [(item.get("symbol_name"), item.get("authored_id")) for item in document["records"]]
    require(actual_records == expected_records, f"semantic metadata record drift: expected={expected_records} actual={actual_records}")
    identity_by_index = {index: item["authored_id"] for index, item in enumerate(document["records"])}
    expected_edges = [
        (item["source_identity"], item["kind"], item["target_identity"], item["resolution"])
        for item in spec["edges"]
    ]
    actual_edges = []
    for edge in document["edges"]:
        require(edge.get("target_record_index") is None, "external metadata edge unexpectedly has a target index")
        actual_edges.append((identity_by_index.get(edge.get("source_record_index")), edge.get("kind"), edge.get("target_id"), edge.get("resolution")))
    require(actual_edges == expected_edges, f"semantic metadata edge drift: expected={expected_edges} actual={actual_edges}")
    return {
        "schema": document["schema"],
        "schema_version": document["schema_version"],
        "producer": producer,
        "analysis_contract": analysis,
        "source": source_receipt,
        "record_count": len(document["records"]),
        "edge_count": len(document["edges"]),
        "sha256": sha256_path(path),
    }


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix=f".{path.name}.tmp.", dir=path.parent)
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    except BaseException:
        temporary.unlink(missing_ok=True)
        raise


def publish_text(path: Path, text: str) -> None:
    atomic_write(path, text.encode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--esp-work-root", required=True, type=Path)
    parser.add_argument("--compiler", type=Path, default=FISICS_ROOT / "fisics")
    parser.add_argument("--contract", type=Path, default=DEFAULT_CONTRACT)
    parser.add_argument("--build-root", type=Path, default=FISICS_ROOT / "build/integration-esp-portable-c")
    args = parser.parse_args()

    contract_path = args.contract.resolve()
    require(contract_path == DEFAULT_CONTRACT.resolve(), "the runner accepts only the checked-in fixed intake contract")
    contract = load_contract(contract_path)
    esp_root = args.esp_work_root.resolve()
    compiler = args.compiler.resolve()
    build_root = args.build_root.resolve()
    require(compiler.is_file() and os.access(compiler, os.X_OK), f"compiler is not executable: {compiler}")
    require((esp_root / ".git").exists(), f"ESP_WORK_ROOT is not a Git checkout: {esp_root}")
    git_root = git(esp_root, "rev-parse", "--show-toplevel", label="ESP Git root").strip()
    require(Path(git_root).resolve() == esp_root, f"ESP_WORK_ROOT is not the exact Git top level: {esp_root}")
    require(build_root == (FISICS_ROOT / "build/integration-esp-portable-c").resolve(), "the runner accepts only the fixed fisiCs build root")
    require(esp_root != build_root and esp_root not in build_root.parents, "build root must not be inside ESP_WORK_ROOT")

    before_status = git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status before")
    before_worktree_fingerprint, before_worktree_entry_count = worktree_fingerprint(esp_root)
    before_inputs = verify_external_inputs(esp_root, contract)
    verify_protocol(esp_root, contract)
    compiler_header = canonical_child(FISICS_ROOT, contract["compiler_header"]["path"], "compiler extension header")
    require(sha256_path(compiler_header) == contract["compiler_header"]["sha256"], "compiler extension-header digest drift")
    vendored_header = canonical_child(esp_root, input_map(contract)["vendored_extension_header"]["path"], "vendored extension header")
    require(compiler_header.read_bytes() == vendored_header.read_bytes(), "compiler and vendored extension headers differ")

    candidates = [verify_manifest(esp_root, candidate) for candidate in contract["candidates"]]
    inputs = input_map(contract)
    behavior = contract["behavior"]
    source = canonical_child(esp_root, inputs[behavior["source_role"]]["path"], "portable source")
    test_source = canonical_child(esp_root, inputs[behavior["test_role"]]["path"], "portable vector source")
    include_path = (esp_root / behavior["include_path"]).resolve()
    require(include_path.is_dir() and esp_root in include_path.parents, "portable include path is unavailable or escapes ESP_WORK_ROOT")
    clang = Path(shutil.which("clang") or "")
    require(clang.is_file(), "strict Clang is required")
    environment = os.environ.copy()
    environment["FISICS_MAX_PROCS"] = "0"

    with tempfile.TemporaryDirectory(prefix="fisics-esp-portable-c-") as temporary_name:
        temporary = Path(temporary_name)
        clang_binary = temporary / "clang.out"
        fisics_binary = temporary / "fisics.out"
        clang_compile = [
            str(clang), "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
            f"-I{include_path}", str(source), str(test_source), "-o", str(clang_binary),
        ]
        fisics_compile = [
            str(compiler), "-std=c99", "--overlay=semantic-metadata",
            f"-I{include_path}", str(source), str(test_source), "-o", str(fisics_binary),
        ]
        run(clang_compile, cwd=FISICS_ROOT, label="strict Clang compile")
        run(fisics_compile, cwd=FISICS_ROOT, label="fisiCs compile", env=environment)
        clang_outputs = [
            run([str(clang_binary)], cwd=temporary, label=f"strict Clang run {index + 1}").stdout
            for index in range(behavior["runs_per_compiler"])
        ]
        fisics_outputs = [
            run([str(fisics_binary)], cwd=temporary, label=f"fisiCs run {index + 1}").stdout
            for index in range(behavior["runs_per_compiler"])
        ]
        all_outputs = clang_outputs + fisics_outputs
        require(all(output == behavior["expected_stdout"] for output in all_outputs), "behavior transcript drift")
        require(len(set(all_outputs)) == 1, "within-compiler or cross-compiler behavior parity failed")

        metadata_path = temporary / "normalized_signal_policy.semantic.json"
        metadata_object = temporary / "normalized_signal_policy.o"
        run(
            [
                str(compiler), "-std=c99", "--overlay=semantic-metadata",
                f"-I{include_path}", "--emit-semantic-metadata-json", str(metadata_path),
                "-c", str(source), "-o", str(metadata_object),
            ],
            cwd=FISICS_ROOT,
            label="semantic metadata export",
            env=environment,
        )
        metadata_receipt = verify_metadata(metadata_path, source, contract)
        compiler_version = run([str(compiler), "--version"], cwd=FISICS_ROOT, label="compiler version").stdout.strip()
        clang_version = run([str(clang), "--version"], cwd=FISICS_ROOT, label="Clang version").stdout.splitlines()[0]

        after_inputs = verify_external_inputs(esp_root, contract)
        after_worktree_fingerprint, after_worktree_entry_count = worktree_fingerprint(esp_root)
        after_status = git(esp_root, "status", "--porcelain=v2", "--branch", label="ESP status after")
        require(after_inputs == before_inputs, "selected ESP input bytes changed during intake")
        require(after_worktree_fingerprint == before_worktree_fingerprint, "ESP worktree bytes changed during intake")
        require(after_worktree_entry_count == before_worktree_entry_count, "ESP worktree entry count changed during intake")
        require(after_status == before_status, "ESP Git status changed during intake")

        receipt = {
            "schema": "fisiCs.esp_portable_c_intake_receipt",
            "schema_version": 1,
            "lane_id": contract["lane_id"],
            "status": "pass",
            "contract": {"path": str(contract_path), "sha256": sha256_path(contract_path)},
            "external_repository": {
                "path": str(esp_root),
                "head": git(esp_root, "rev-parse", "HEAD", label="ESP HEAD").strip(),
                "dirty": bool(git(esp_root, "status", "--porcelain", label="ESP dirty status").strip()),
                "status_sha256": sha256_bytes(before_status.encode("utf-8")),
                "worktree_entry_count": before_worktree_entry_count,
                "worktree_fingerprint_sha256": before_worktree_fingerprint,
                "unchanged_during_intake": True,
            },
            "compiler": {"path": str(compiler), "version": compiler_version, "sha256": sha256_path(compiler)},
            "clang": {"path": str(clang), "version": clang_version},
            "inputs": [
                {"role": item["role"], "path": item["path"], "sha256": before_inputs[item["role"]]}
                for item in contract["external_inputs"]
            ],
            "behavior": {
                "normalization_cases": behavior["normalization_cases"],
                "publication_cases": behavior["publication_cases"],
                "runs_per_compiler": behavior["runs_per_compiler"],
                "expected_stdout": behavior["expected_stdout"],
                "stdout_sha256": sha256_bytes(behavior["expected_stdout"].encode("utf-8")),
                "clang_repeat_identical": True,
                "fisics_repeat_identical": True,
                "cross_compiler_identical": True,
            },
            "semantic_metadata": metadata_receipt,
            "candidates": candidates,
            "claim_boundary": {
                "compiler_validated_host_behavior": "established",
                "control_compiler_host_behavior": "established",
                "host_differential_parity": "established",
                "semantic_metadata": "established",
                "vendor_firmware_build": "external_receipt_validated",
                "hardware_presence": "not_assessed",
                "wiring": "not_assessed",
                "driver_execution": "not_assessed",
                "calibration": "not_assessed",
                "deployment": "not_assessed",
                "runtime_observation": "not_assessed",
                "physical_acceptance": "not_assessed",
            },
        }
        receipt_bytes = (json.dumps(receipt, indent=2, sort_keys=True) + "\n").encode("utf-8")
        metadata_bytes = metadata_path.read_bytes()

    publish_text(build_root / "clang.run-1.txt", clang_outputs[0])
    publish_text(build_root / "clang.run-2.txt", clang_outputs[1])
    publish_text(build_root / "fisics.run-1.txt", fisics_outputs[0])
    publish_text(build_root / "fisics.run-2.txt", fisics_outputs[1])
    atomic_write(build_root / "normalized_signal_policy.semantic.json", metadata_bytes)
    atomic_write(build_root / "intake.receipt.json", receipt_bytes)
    print(
        f"ESP portable C intake lane={contract['lane_id']} normalization={behavior['normalization_cases']} "
        f"publication={behavior['publication_cases']} clang=fisiCs parity=PASS "
        f"external_tree=UNCHANGED result=PASS"
    )
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as exc:
        print(f"ESP portable C intake: FAIL: {exc}", file=os.sys.stderr)
        raise SystemExit(1)
