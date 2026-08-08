#!/usr/bin/env python3
"""Run a bounded, noncanonical compiler canary against signed ``os-dev`` input.

The checked-in contract intentionally pins a tag, its object, source hash, and
the compiler accepted by the downstream replay.  This runner extracts the
source from the immutable tag into a temporary directory; it never writes into
the downstream checkout and does not claim OS, loader, or guest authority.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


LANE_ROOT = Path(__file__).resolve().parent
FISICS_ROOT = LANE_ROOT.parent.parent
CONTRACT_PATH = LANE_ROOT / "canaries/edu48_simulation_kernel_canary.json"
DRIVER_PATH = LANE_ROOT / "canaries/edu48_simulation_kernel_driver.c"
EXPECTED_PATH = LANE_ROOT / "canaries/edu48_simulation_kernel.stdout"
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


def load_contract(path: Path) -> dict[str, Any]:
    require_file(path, "canary contract")
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1:
        raise RuntimeError("canary contract schema_version must be 1")
    origin = data.get("origin")
    if not isinstance(origin, dict):
        raise RuntimeError("canary contract origin must be an object")
    for key in ("tag", "tag_object", "commit", "source_path", "source_sha256"):
        if not isinstance(origin.get(key), str) or not origin[key]:
            raise RuntimeError(f"canary origin {key} must be a non-empty string")
    accepted = data.get("accepted_compiler")
    if not isinstance(accepted, dict):
        raise RuntimeError("canary accepted_compiler must be an object")
    for key in ("relative_path", "sha256"):
        if not isinstance(accepted.get(key), str) or not accepted[key]:
            raise RuntimeError(f"canary accepted_compiler {key} must be a non-empty string")
    if not isinstance(data.get("candidate_object_sha256"), str):
        raise RuntimeError("canary candidate_object_sha256 must be a string")
    if not isinstance(data.get("expected_stdout"), str):
        raise RuntimeError("canary expected_stdout must be a string")
    if not isinstance(data.get("object_contract"), dict):
        raise RuntimeError("canary object_contract must be an object")
    return data


def compiler_version(compiler: Path) -> str:
    return run([str(compiler), "--version"], cwd=FISICS_ROOT, label="compiler version").stdout.strip()


def compile_and_run_runtime(
    *, compiler: Path, source: Path, output: Path, label: str
) -> str:
    run(
        [str(compiler), str(source), str(DRIVER_PATH), "-o", str(output)],
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
    checkout = args.os_dev_root.resolve()
    compiler = args.compiler.resolve()
    require_file(compiler, "candidate compiler")
    require_file(DRIVER_PATH, "canary runtime driver")
    require_file(EXPECTED_PATH, "canary expected transcript")
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

    source_bytes = run(
        ["git", "show", f"{origin['tag']}:{origin['source_path']}"],
        cwd=checkout,
        label="immutable source extraction",
    ).stdout.encode("utf-8")
    require_equal(sha256_bytes(source_bytes), origin["source_sha256"], "immutable source hash")
    live_source = checkout / origin["source_path"]
    require_file(live_source, "live os-dev source")
    require_equal(sha256_path(live_source), origin["source_sha256"], "live source hash")

    accepted_compiler = (checkout / contract["accepted_compiler"]["relative_path"]).resolve()
    require_file(accepted_compiler, "accepted os-dev compiler")
    require_equal(sha256_path(accepted_compiler), contract["accepted_compiler"]["sha256"], "accepted compiler hash")

    readobj = shutil.which("llvm-readobj")
    objdump = shutil.which("llvm-objdump")
    if not readobj or not objdump:
        raise RuntimeError("llvm-readobj and llvm-objdump are required for the canary")
    expected_stdout = EXPECTED_PATH.read_text(encoding="utf-8")
    require_equal(contract["expected_stdout"], expected_stdout, "contract transcript")

    with tempfile.TemporaryDirectory(prefix="fisics-os-dev-edu48-") as directory:
        root = Path(directory)
        source = root / "simulation_kernel.c"
        source.write_bytes(source_bytes)
        candidate_a = root / "candidate-a.o"
        candidate_b = root / "candidate-b.o"
        accepted_object = root / "accepted.o"
        for output in (candidate_a, candidate_b):
            run(
                [str(compiler), "--target", "x86_64-unknown-none", "-c", str(source), "-o", str(output)],
                cwd=FISICS_ROOT,
                label=f"candidate freestanding compile {output.name}",
            )
        run(
            [str(accepted_compiler), "--target", "x86_64-unknown-none", "-c", str(source), "-o", str(accepted_object)],
            cwd=FISICS_ROOT,
            label="accepted compiler freestanding compile",
        )
        if candidate_a.read_bytes() != candidate_b.read_bytes():
            raise RuntimeError("candidate repeated freestanding objects are not byte-identical")
        require_equal(sha256_path(candidate_a), contract["candidate_object_sha256"], "candidate object hash")
        if candidate_a.read_bytes() != accepted_object.read_bytes():
            raise RuntimeError("candidate and accepted compiler objects differ")
        object_verification = verify_object(
            object_path=candidate_a,
            readobj=Path(readobj),
            objdump=Path(objdump),
            contract=contract["object_contract"],
        )
        fisics_stdout = compile_and_run_runtime(
            compiler=compiler,
            source=source,
            output=root / "candidate.out",
            label="candidate",
        )
        clang = Path(shutil.which("clang") or "")
        if not clang.is_file():
            raise RuntimeError("clang is required for the canary runtime differential")
        clang_output = root / "clang.out"
        run(
            ["clang", "-std=c99", "-Wall", "-Wextra", "-Wpedantic", str(source), str(DRIVER_PATH), "-o", str(clang_output)],
            cwd=FISICS_ROOT,
            label="Clang runtime compile",
        )
        clang_stdout = run([str(clang_output)], cwd=root, label="Clang runtime execution").stdout
        sanitized = root / "clang-sanitized.out"
        run(
            ["clang", "-std=c99", "-Wall", "-Wextra", "-Wpedantic", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", str(source), str(DRIVER_PATH), "-o", str(sanitized)],
            cwd=FISICS_ROOT,
            label="Clang sanitizer runtime compile",
        )
        sanitizer_stdout = run([str(sanitized)], cwd=root, label="Clang sanitizer runtime execution").stdout
        for label, stdout in (("candidate", fisics_stdout), ("clang", clang_stdout), ("clang-sanitizer", sanitizer_stdout)):
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
            "object_sha256": sha256_path(candidate_a),
            "object_verification": object_verification,
            "runtime_stdout_sha256": sha256_bytes(fisics_stdout.encode("utf-8")),
        }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"OS-DEV canary lane={contract['lane_id']} source=immutable-tag candidate=accepted "
        f"runtime=clang+asan trust={signature_status} result=PASS"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
