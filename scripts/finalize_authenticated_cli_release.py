#!/usr/bin/env python3
"""Finalize and independently verify authenticated fisiCs CLI release evidence."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import tarfile
import tempfile
import zipfile
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(argv: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        argv, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False
    )


def require_command(argv: list[str], label: str) -> str:
    result = run(argv)
    if result.returncode:
        raise SystemExit(f"ERROR: {label} failed")
    return result.stdout


def read_checksum(path: Path, artifact: Path) -> str:
    fields = path.read_text(encoding="utf-8").split()
    if len(fields) < 2 or fields[0] != sha256(artifact) or Path(fields[-1]).name != artifact.name:
        raise SystemExit(f"ERROR: checksum does not bind {artifact.name}")
    return fields[0]


def extract_binary(artifact: Path, destination: Path, kind: str) -> Path:
    if kind == "zip":
        with zipfile.ZipFile(artifact) as archive:
            archive.extractall(destination)
    else:
        with tarfile.open(artifact, "r:gz") as archive:
            archive.extractall(destination, filter="data")
    binaries = sorted(destination.glob("*/bin/fisics"))
    if len(binaries) != 1 or not binaries[0].is_file():
        raise SystemExit(f"ERROR: {artifact.name} does not contain exactly one fisiCs CLI")
    binaries[0].chmod(binaries[0].stat().st_mode | 0o111)
    return binaries[0]


def signature_identity(binary: Path) -> tuple[str, str]:
    require_command(
        ["/usr/bin/codesign", "--verify", "--deep", "--strict", "--verbose=4", str(binary)],
        "strict Developer ID signature verification",
    )
    details = require_command(
        ["/usr/bin/codesign", "-d", "--verbose=4", str(binary)],
        "Developer ID signature readback",
    )
    fields: dict[str, str] = {}
    authorities: list[str] = []
    for line in details.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if key == "Authority":
            authorities.append(value)
        else:
            fields[key] = value
    identity = next(
        (value for value in authorities if value.startswith("Developer ID Application: ")),
        "",
    )
    team_id = fields.get("TeamIdentifier", "")
    if not identity or not team_id or team_id == "not set":
        raise SystemExit("ERROR: CLI is not signed by a Developer ID Application identity")
    return identity, team_id


def parse_manifest(path: Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value
    return fields


def manifest_text(
    *, artifact: Path, artifact_sha: str, fmt: str, version: str, platform: str,
    arch: str, channel: str, identity: str, team_id: str, submission_id: str,
) -> str:
    return "\n".join(
        [
            "product=fisiCs",
            "program=fisiCs",
            f"version={version}",
            f"channel={channel}",
            f"platform={platform}",
            f"arch={arch}",
            f"format={fmt}",
            "signed=1",
            "notarized=1",
            "stapling=not_applicable",
            f"artifact={artifact.name}",
            f"sha256={artifact_sha}",
            f"developer_id_identity={identity}",
            f"team_id={team_id}",
            "notary_status=Accepted",
            f"notary_submission_id={submission_id}",
            "",
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--stage-dir", type=Path, required=True)
    parser.add_argument("--zip", type=Path, required=True)
    parser.add_argument("--tar-gz", type=Path, required=True)
    parser.add_argument("--zip-checksum", type=Path, required=True)
    parser.add_argument("--tar-gz-checksum", type=Path, required=True)
    parser.add_argument("--zip-manifest", type=Path, required=True)
    parser.add_argument("--tar-gz-manifest", type=Path, required=True)
    parser.add_argument("--compat-manifest", type=Path, required=True)
    parser.add_argument("--notary-json", type=Path, required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--arch", required=True)
    parser.add_argument("--channel", required=True)
    args = parser.parse_args()

    stage_binary = args.stage_dir / "bin/fisics"
    notary = json.loads(args.notary_json.read_text(encoding="utf-8"))
    if notary.get("status") != "Accepted" or not notary.get("id"):
        raise SystemExit("ERROR: notarization response is not Accepted")
    identity, team_id = signature_identity(stage_binary)
    expected: dict[Path, str] = {}
    artifacts = (
        (args.zip, args.zip_checksum, args.zip_manifest, "zip"),
        (args.tar_gz, args.tar_gz_checksum, args.tar_gz_manifest, "tar.gz"),
    )
    with tempfile.TemporaryDirectory(prefix="fisics-authenticated-release-") as temp:
        root = Path(temp)
        for index, (artifact, checksum, manifest, fmt) in enumerate(artifacts):
            artifact_sha = read_checksum(checksum, artifact)
            extracted = extract_binary(artifact, root / str(index), fmt)
            if extracted.read_bytes() != stage_binary.read_bytes():
                raise SystemExit(f"ERROR: {artifact.name} changed the signed CLI bytes")
            extracted_identity, extracted_team = signature_identity(extracted)
            if (extracted_identity, extracted_team) != (identity, team_id):
                raise SystemExit(f"ERROR: {artifact.name} signature identity drifted")
            expected[manifest] = manifest_text(
                artifact=artifact, artifact_sha=artifact_sha, fmt=fmt,
                version=args.version, platform=args.platform, arch=args.arch,
                channel=args.channel, identity=identity, team_id=team_id,
                submission_id=str(notary["id"]),
            )
    if args.verify_only:
        for path, text in expected.items():
            if not path.is_file() or parse_manifest(path) != parse_manifest_text(text):
                raise SystemExit(f"ERROR: authenticated manifest drifted: {path.name}")
    else:
        for path, text in expected.items():
            path.write_text(text, encoding="utf-8")
        args.compat_manifest.write_text(expected[args.zip_manifest], encoding="utf-8")
    print("authenticated CLI release evidence verified")
    return 0


def parse_manifest_text(text: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in text.splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            fields[key] = value
    return fields


if __name__ == "__main__":
    raise SystemExit(main())
