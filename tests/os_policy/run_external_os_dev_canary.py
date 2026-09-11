#!/usr/bin/env python3
"""Run a bounded, noncanonical compiler canary against signed ``os-dev`` input.

The checked-in contract pins a tag, one or more source hashes, and the compiler
accepted by the downstream replay. This runner extracts those sources from the
immutable tag into a temporary directory; it never writes into the downstream
checkout and does not claim OS, loader, or guest authority.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


LANE_ROOT = Path(__file__).resolve().parent
FISICS_ROOT = LANE_ROOT.parent.parent
CONTRACT_PATH = LANE_ROOT / "canaries/edu48_simulation_kernel_canary.json"
sys.path.insert(0, str(LANE_ROOT))
from verify_object import verify_object  # noqa: E402


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def sha256_path(path: Path) -> str:
    return sha256_bytes(path.read_bytes())


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"{label} is unavailable: {path}")


def run(command: list[str], *, cwd: Path, label: str) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"{label} failed with exit {completed.returncode}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    return completed


def git(checkout: Path, *args: str, label: str) -> str:
    return run(["git", *args], cwd=checkout, label=label).stdout.strip()


def require_equal(actual: str, expected: str, label: str) -> None:
    if actual != expected:
        raise RuntimeError(f"{label} mismatch: expected={expected} actual={actual}")


def canary_asset_path(value: str, label: str) -> Path:
    path = Path(value)
    if path.is_absolute() or ".." in path.parts:
        raise RuntimeError(f"{label} must stay within the canaries directory: {value}")
    resolved = (LANE_ROOT / "canaries" / path).resolve()
    canaries = (LANE_ROOT / "canaries").resolve()
    if canaries not in resolved.parents:
        raise RuntimeError(f"{label} escapes the canaries directory: {value}")
    return resolved


def contract_sources(data: dict[str, Any]) -> list[dict[str, Any]]:
    """Normalize v1 single-source and v2 multi-source contracts."""
    origin = data["origin"]
    if data["schema_version"] == 1:
        return [{
            "path": origin["source_path"],
            "sha256": origin["source_sha256"],
            "candidate_object_sha256": data["candidate_object_sha256"],
            "object_contract": data["object_contract"],
        }]
    sources = data.get("sources")
    if not isinstance(sources, list) or len(sources) < 2:
        raise RuntimeError("schema_version 2 requires at least two sources")
    paths: set[str] = set()
    for index, source in enumerate(sources):
        if not isinstance(source, dict):
            raise RuntimeError(f"source {index} must be an object")
        for key in ("path", "sha256", "candidate_object_sha256", "accepted_object_sha256"):
            if not isinstance(source.get(key), str) or not source[key]:
                raise RuntimeError(f"source {index} {key} must be a non-empty string")
        if not isinstance(source.get("object_contract"), dict):
            raise RuntimeError(f"source {index} object_contract must be an object")
        path = Path(source["path"])
        if path.is_absolute() or ".." in path.parts:
            raise RuntimeError(f"source {index} path must be repository-relative")
        if source["path"] in paths:
            raise RuntimeError(f"source {index} path duplicates {source['path']}")
        paths.add(source["path"])
        delta = source.get("approved_object_delta")
        if delta is not None and delta != {"kind": "static_bss_address_relocation"}:
            raise RuntimeError(
                f"source {index} approved_object_delta is not the one supported delta"
            )
    return sources


def bss_relocation_counts(object_path: Path, readobj: Path) -> dict[str, int]:
    """Return relocation-type counts for relocations that target ``.bss``."""
    output = run(
        [str(readobj), "--relocations", str(object_path)],
        cwd=FISICS_ROOT,
        label=f"relocation inspection {object_path.name}",
    ).stdout
    return {
        relocation: len(re.findall(rf"{relocation} \.bss\b", output))
        for relocation in ("R_X86_64_64", "R_X86_64_PC32")
    }


def verify_approved_object_delta(
    *, candidate: Path, accepted: Path, readobj: Path, delta: dict[str, Any]
) -> dict[str, Any]:
    """Verify the only permitted non-byte-identical object relationship."""
    if delta != {"kind": "static_bss_address_relocation"}:
        raise RuntimeError("unsupported approved object delta")
    candidate_counts = bss_relocation_counts(candidate, readobj)
    accepted_counts = bss_relocation_counts(accepted, readobj)
    if candidate_counts != {"R_X86_64_64": 0, "R_X86_64_PC32": 1}:
        raise RuntimeError(f"candidate static-storage relocation delta mismatch: {candidate_counts}")
    if accepted_counts != {"R_X86_64_64": 1, "R_X86_64_PC32": 0}:
        raise RuntimeError(f"accepted static-storage relocation delta mismatch: {accepted_counts}")
    return {
        "kind": delta["kind"],
        "candidate_bss_relocations": candidate_counts,
        "accepted_bss_relocations": accepted_counts,
    }


def load_contract(path: Path) -> dict[str, Any]:
    require_file(path, "canary contract")
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") not in (1, 2):
        raise RuntimeError("canary contract schema_version must be 1 or 2")
    origin = data.get("origin")
    if not isinstance(origin, dict):
        raise RuntimeError("canary contract origin must be an object")
    for key in ("tag", "tag_object", "commit"):
        if not isinstance(origin.get(key), str) or not origin[key]:
            raise RuntimeError(f"canary origin {key} must be a non-empty string")
    accepted = data.get("accepted_compiler")
    if not isinstance(accepted, dict):
        raise RuntimeError("canary accepted_compiler must be an object")
    for key in ("relative_path", "sha256"):
        if not isinstance(accepted.get(key), str) or not accepted[key]:
            raise RuntimeError(f"canary accepted_compiler {key} must be a non-empty string")
    if data["schema_version"] == 1:
        for key in ("source_path", "source_sha256"):
            if not isinstance(origin.get(key), str) or not origin[key]:
                raise RuntimeError(f"canary origin {key} must be a non-empty string")
        if not isinstance(data.get("candidate_object_sha256"), str):
            raise RuntimeError("canary candidate_object_sha256 must be a string")
    if not isinstance(data.get("expected_stdout"), str):
        raise RuntimeError("canary expected_stdout must be a string")
    for key in ("runtime_driver", "expected_stdout_path"):
        if not isinstance(data.get(key), str) or not data[key]:
            raise RuntimeError(f"canary {key} must be a non-empty string")
        require_file(canary_asset_path(data[key], f"canary {key}"), f"canary {key}")
    if data["schema_version"] == 1 and not isinstance(data.get("object_contract"), dict):
        raise RuntimeError("canary object_contract must be an object")
    contract_sources(data)
    return data


def compiler_version(compiler: Path) -> str:
    return run([str(compiler), "--version"], cwd=FISICS_ROOT, label="compiler version").stdout.strip()


def compile_and_run_runtime(
    *, compiler: Path, sources: list[Path], driver: Path, output: Path, label: str
) -> str:
    run(
        [str(compiler), *(str(source) for source in sources), str(driver), "-o", str(output)],
        cwd=FISICS_ROOT,
        label=f"{label} runtime compile",
    )
    completed = run([str(output)], cwd=output.parent, label=f"{label} runtime execution")
    return completed.stdout


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--os-dev-root",
        required=True,
        type=Path,
        help="read-only path to the os-dev checkout containing the signed EDU-48 tag",
    )
    parser.add_argument("--compiler", type=Path, default=FISICS_ROOT / "fisics")
    parser.add_argument("--contract", type=Path, default=CONTRACT_PATH)
    parser.add_argument("--report", type=Path)
    parser.add_argument(
        "--allowed-signers",
        type=Path,
        help=(
            "explicit read-only Git SSH allowed-signers file used only for "
            "this canary's tag verification; it does not change global Git config"
        ),
    )
    parser.add_argument(
        "--allow-unverified-tag",
        action="store_true",
        help=(
            "run only structural/compiler drift evidence when this host lacks "
            "the configured SSH allowed-signers trust anchor; the report and "
            "summary remain explicitly unverified"
        ),
    )
    args = parser.parse_args()

    contract = load_contract(args.contract)
    origin = contract["origin"]
    sources_contract = contract_sources(contract)
    driver_path = canary_asset_path(contract["runtime_driver"], "canary runtime_driver")
    expected_path = canary_asset_path(
        contract["expected_stdout_path"], "canary expected_stdout_path"
    )
    checkout = args.os_dev_root.resolve()
    compiler = args.compiler.resolve()
    require_file(compiler, "candidate compiler")
    if not (checkout / ".git").exists():
        raise RuntimeError(f"os-dev root is not a Git checkout: {checkout}")
    require_equal(git(checkout, "rev-parse", "--show-toplevel", label="os-dev root"), str(checkout), "os-dev root")
    require_equal(git(checkout, "rev-parse", f"{origin['tag']}^{{tag}}", label="tag object"), origin["tag_object"], "signed tag object")
    require_equal(git(checkout, "rev-parse", f"{origin['tag']}^{{commit}}", label="tag commit"), origin["commit"], "signed tag commit")
    signature_status = "verified"
    signature_command = ["git", "tag", "-v", origin["tag"]]
    if args.allowed_signers:
        allowed_signers = args.allowed_signers.resolve()
        require_file(allowed_signers, "canary SSH allowed-signers file")
        signature_command = [
            "git",
            "-c",
            f"gpg.ssh.allowedSignersFile={allowed_signers}",
            "tag",
            "-v",
            origin["tag"],
        ]
    signature = subprocess.run(
        signature_command,
        cwd=checkout,
        text=True,
        capture_output=True,
        check=False,
    )
    if signature.returncode != 0:
        if not args.allow_unverified_tag:
            raise RuntimeError(
                "signed tag verification failed; provide this host's trusted "
                "SSH allowed-signers configuration or explicitly request the "
                "noncanonical --allow-unverified-tag structural-only run\n"
                f"stdout:\n{signature.stdout}\nstderr:\n{signature.stderr}"
            )
        signature_status = "unverified"

    accepted_compiler = (checkout / contract["accepted_compiler"]["relative_path"]).resolve()
    require_file(accepted_compiler, "accepted os-dev compiler")
    require_equal(sha256_path(accepted_compiler), contract["accepted_compiler"]["sha256"], "accepted compiler hash")

    readobj = shutil.which("llvm-readobj")
    objdump = shutil.which("llvm-objdump")
    if not readobj or not objdump:
        raise RuntimeError("llvm-readobj and llvm-objdump are required for the canary")
    expected_stdout = expected_path.read_text(encoding="utf-8")
    require_equal(contract["expected_stdout"], expected_stdout, "contract transcript")

    with tempfile.TemporaryDirectory(prefix=f"fisics-os-dev-{contract['lane_id']}-") as directory:
        root = Path(directory)
        sources: list[Path] = []
        source_reports: list[dict[str, Any]] = []
        for index, source_contract in enumerate(sources_contract):
            source_bytes = run(
                ["git", "show", f"{origin['tag']}:{source_contract['path']}"],
                cwd=checkout,
                label=f"immutable source extraction {source_contract['path']}",
            ).stdout.encode("utf-8")
            require_equal(sha256_bytes(source_bytes), source_contract["sha256"], f"immutable source hash {source_contract['path']}")
            source = root / source_contract["path"]
            source.parent.mkdir(parents=True, exist_ok=True)
            source.write_bytes(source_bytes)
            sources.append(source)
            candidate_a = root / f"candidate-{index}-a.o"
            candidate_b = root / f"candidate-{index}-b.o"
            accepted_object = root / f"accepted-{index}.o"
            for output in (candidate_a, candidate_b):
                run([str(compiler), "--target", "x86_64-unknown-none", "-c", str(source), "-o", str(output)], cwd=FISICS_ROOT, label=f"candidate freestanding compile {source.name} {output.name}")
            run([str(accepted_compiler), "--target", "x86_64-unknown-none", "-c", str(source), "-o", str(accepted_object)], cwd=FISICS_ROOT, label=f"accepted compiler freestanding compile {source.name}")
            if candidate_a.read_bytes() != candidate_b.read_bytes():
                raise RuntimeError(f"candidate repeated freestanding objects are not byte-identical: {source_contract['path']}")
            require_equal(sha256_path(candidate_a), source_contract["candidate_object_sha256"], f"candidate object hash {source_contract['path']}")
            accepted_hash = source_contract.get(
                "accepted_object_sha256", source_contract["candidate_object_sha256"]
            )
            require_equal(
                sha256_path(accepted_object), accepted_hash,
                f"accepted object hash {source_contract['path']}",
            )
            delta = source_contract.get("approved_object_delta")
            if delta is None:
                if candidate_a.read_bytes() != accepted_object.read_bytes():
                    raise RuntimeError(f"candidate and accepted compiler objects differ: {source_contract['path']}")
                delta_verification = {"kind": "strict-byte-identity"}
            else:
                delta_verification = verify_approved_object_delta(
                    candidate=candidate_a,
                    accepted=accepted_object,
                    readobj=Path(readobj),
                    delta=delta,
                )
            source_reports.append({
                "path": source_contract["path"],
                "sha256": source_contract["sha256"],
                "object_sha256": sha256_path(candidate_a),
                "accepted_object_sha256": sha256_path(accepted_object),
                "object_delta_verification": delta_verification,
                "object_verification": verify_object(
                    object_path=candidate_a,
                    readobj=Path(readobj),
                    objdump=Path(objdump),
                    contract=source_contract["object_contract"],
                ),
            })
        fisics_stdout = compile_and_run_runtime(
            compiler=compiler,
            sources=sources,
            driver=driver_path,
            output=root / "candidate.out",
            label="candidate",
        )
        clang = Path(shutil.which("clang") or "")
        if not clang.is_file():
            raise RuntimeError("clang is required for the canary runtime differential")
        clang_output = root / "clang.out"
        run(
            ["clang", "-std=c99", "-Wall", "-Wextra", "-Wpedantic", *(str(source) for source in sources), str(driver_path), "-o", str(clang_output)],
            cwd=FISICS_ROOT,
            label="Clang runtime compile",
        )
        clang_stdout = run([str(clang_output)], cwd=root, label="Clang runtime execution").stdout
        sanitized = root / "clang-sanitized.out"
        run(
            ["clang", "-std=c99", "-Wall", "-Wextra", "-Wpedantic", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", *(str(source) for source in sources), str(driver_path), "-o", str(sanitized)],
            cwd=FISICS_ROOT,
            label="Clang sanitizer runtime compile",
        )
        sanitizer_stdout = run([str(sanitized)], cwd=root, label="Clang sanitizer runtime execution").stdout
        gcc = Path(shutil.which("gcc-16") or shutil.which("gcc") or "")
        if not gcc.is_file():
            raise RuntimeError("GNU GCC is required for the canary runtime differential")
        gcc_output = root / "gcc.out"
        run(
            [str(gcc), "-std=c99", "-Wall", "-Wextra", "-Wpedantic", *(str(source) for source in sources), str(driver_path), "-o", str(gcc_output)],
            cwd=FISICS_ROOT,
            label="GNU GCC runtime compile",
        )
        gcc_stdout = run([str(gcc_output)], cwd=root, label="GNU GCC runtime execution").stdout
        for label, stdout in (("candidate", fisics_stdout), ("clang", clang_stdout), ("clang-sanitizer", sanitizer_stdout), ("gcc", gcc_stdout)):
            require_equal(stdout, expected_stdout, f"{label} runtime transcript")
        report = {
            "status": "pass",
            "lane_id": contract["lane_id"],
            "origin": origin,
            "signature_status": signature_status,
            "allowed_signers": str(args.allowed_signers.resolve()) if args.allowed_signers else "",
            "live_head": git(checkout, "rev-parse", "HEAD", label="live head"),
            "live_dirty": bool(git(checkout, "status", "--porcelain", label="live dirty status")),
            "candidate_compiler": {"path": str(compiler), "sha256": sha256_path(compiler), "version": compiler_version(compiler)},
            "accepted_compiler": {"path": str(accepted_compiler), "sha256": sha256_path(accepted_compiler), "version": compiler_version(accepted_compiler)},
            "sources": source_reports,
            "runtime_stdout_sha256": sha256_bytes(fisics_stdout.encode("utf-8")),
        }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"OS-DEV canary lane={contract['lane_id']} source=immutable-tag candidate=accepted "
        f"runtime=clang+gcc+asan trust={signature_status} result=PASS"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
