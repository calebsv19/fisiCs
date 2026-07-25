#!/usr/bin/env python3
"""Run the compiler-owned OS Policy validation lane."""

from __future__ import annotations

import argparse
import hashlib
import json
import platform
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

from verify_object import verify_object


LANE_ROOT = Path(__file__).resolve().parent
FISICS_ROOT = LANE_ROOT.parent.parent
DEFAULT_MANIFEST = LANE_ROOT / "manifest.json"
DEFAULT_BUILD_ROOT = FISICS_ROOT / "build" / "os_policy"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def run(
    command: list[str],
    *,
    cwd: Path,
    timeout: int = 120,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=cwd,
        text=True,
        capture_output=True,
        timeout=timeout,
        check=False,
    )


def require_success(
    completed: subprocess.CompletedProcess[str],
    *,
    label: str,
) -> None:
    if completed.returncode == 0:
        return
    raise RuntimeError(
        f"{label} failed with exit {completed.returncode}\n"
        f"stdout:\n{completed.stdout}\n"
        f"stderr:\n{completed.stderr}"
    )


def load_manifest(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 2:
        raise RuntimeError("OS-P manifest schema_version must be 2")
    for key in ("lane_id", "lane_version", "target"):
        if not isinstance(data.get(key), str) or not data[key]:
            raise RuntimeError(f"OS-P manifest {key} must be a non-empty string")
    guest_harness = data.get("guest_harness")
    if not isinstance(guest_harness, dict):
        raise RuntimeError("OS-P manifest guest_harness must be an object")
    required_harness_fields = (
        "boot_source",
        "entry_source",
        "linker_script",
        "kernel_load_address",
        "max_kernel_sectors",
        "qemu_machine",
        "qemu_cpu",
        "memory_mb",
        "smp",
    )
    missing_harness = [
        key for key in required_harness_fields if key not in guest_harness
    ]
    if missing_harness:
        raise RuntimeError(
            f"OS-P manifest guest_harness missing fields: {missing_harness}"
        )
    for key in ("boot_source", "entry_source", "linker_script"):
        if not isinstance(guest_harness[key], str) or not guest_harness[key]:
            raise RuntimeError(f"OS-P guest_harness {key} must be a non-empty string")
    for key in (
        "kernel_load_address",
        "max_kernel_sectors",
        "memory_mb",
        "smp",
    ):
        if not isinstance(guest_harness[key], int) or guest_harness[key] < 1:
            raise RuntimeError(f"OS-P guest_harness {key} must be a positive integer")
    for key in ("qemu_machine", "qemu_cpu"):
        if not isinstance(guest_harness[key], str) or not guest_harness[key]:
            raise RuntimeError(f"OS-P guest_harness {key} must be a non-empty string")
    cases = data.get("cases")
    if not isinstance(cases, list) or not cases:
        raise RuntimeError("OS-P manifest must contain at least one case")
    ids = [case.get("id") for case in cases]
    if any(not isinstance(case_id, str) or not case_id for case_id in ids):
        raise RuntimeError("every OS-P case must have a non-empty string id")
    if len(ids) != len(set(ids)):
        raise RuntimeError("OS-P case ids must be unique")
    required_case_fields = (
        "description",
        "source",
        "runtime_driver",
        "expected_stdout",
        "expected_exit",
        "introduced_in",
        "provenance",
        "object_contract",
        "guest_contract",
    )
    for case in cases:
        missing = [key for key in required_case_fields if key not in case]
        if missing:
            raise RuntimeError(f"OS-P case {case['id']} missing fields: {missing}")
        if not isinstance(case["expected_exit"], int):
            raise RuntimeError(f"OS-P case {case['id']} expected_exit must be an integer")
        if not isinstance(case["provenance"], dict):
            raise RuntimeError(f"OS-P case {case['id']} provenance must be an object")
        if not isinstance(case["object_contract"], dict):
            raise RuntimeError(f"OS-P case {case['id']} object_contract must be an object")
        if not isinstance(case["introduced_in"], str) or not case["introduced_in"]:
            raise RuntimeError(f"OS-P case {case['id']} introduced_in must be a string")
        guest_contract = case["guest_contract"]
        if not isinstance(guest_contract, dict):
            raise RuntimeError(f"OS-P case {case['id']} guest_contract must be an object")
        required_guest_fields = (
            "adapter_source",
            "expected_serial",
            "expected_exit",
            "debug_exit_value",
            "timeout_seconds",
            "repeat_runs",
            "parity_artifact",
        )
        missing_guest = [
            key for key in required_guest_fields if key not in guest_contract
        ]
        if missing_guest:
            raise RuntimeError(
                f"OS-P case {case['id']} guest_contract missing fields: {missing_guest}"
            )
        for key in ("adapter_source", "expected_serial"):
            if not isinstance(guest_contract[key], str) or not guest_contract[key]:
                raise RuntimeError(
                    f"OS-P case {case['id']} guest_contract {key} must be a string"
                )
        for key in (
            "expected_exit",
            "debug_exit_value",
            "timeout_seconds",
            "repeat_runs",
        ):
            if not isinstance(guest_contract[key], int):
                raise RuntimeError(
                    f"OS-P case {case['id']} guest_contract {key} must be an integer"
                )
        if guest_contract["timeout_seconds"] < 1:
            raise RuntimeError(
                f"OS-P case {case['id']} guest timeout_seconds must be positive"
            )
        if guest_contract["repeat_runs"] < 2:
            raise RuntimeError(
                f"OS-P case {case['id']} guest repeat_runs must be at least 2"
            )
        derived_exit = (guest_contract["debug_exit_value"] << 1) | 1
        if guest_contract["expected_exit"] != derived_exit:
            raise RuntimeError(
                f"OS-P case {case['id']} guest expected_exit must equal "
                f"(debug_exit_value << 1) | 1"
            )
        if guest_contract["parity_artifact"] != "serial_transcript":
            raise RuntimeError(
                f"OS-P case {case['id']} guest parity_artifact must be "
                "serial_transcript"
            )
        scalar_sse2 = guest_contract.get("scalar_sse2", False)
        if not isinstance(scalar_sse2, bool):
            raise RuntimeError(
                f"OS-P case {case['id']} guest scalar_sse2 must be a boolean"
            )
    return data


def lane_path(relative_value: str, *, must_exist: bool = True) -> Path:
    raw = Path(relative_value)
    if raw.is_absolute():
        raise RuntimeError(f"OS-P manifest path must be relative: {relative_value}")
    resolved = (LANE_ROOT / raw).resolve()
    try:
        resolved.relative_to(LANE_ROOT)
    except ValueError as exc:
        raise RuntimeError(f"OS-P manifest path escapes lane root: {relative_value}") from exc
    if must_exist and not resolved.is_file():
        raise RuntimeError(f"OS-P manifest file does not exist: {relative_value}")
    return resolved


def select_cases(manifest: dict[str, Any], case_filter: str) -> list[dict[str, Any]]:
    cases = list(manifest["cases"])
    if case_filter:
        cases = [case for case in cases if case_filter in case["id"]]
    if not cases:
        raise RuntimeError(f"no OS-P cases selected for filter {case_filter!r}")
    return cases


def llvm_tool_paths() -> tuple[Path, Path, str]:
    bindir_result = run(["llvm-config", "--bindir"], cwd=FISICS_ROOT)
    require_success(bindir_result, label="llvm-config --bindir")
    version_result = run(["llvm-config", "--version"], cwd=FISICS_ROOT)
    require_success(version_result, label="llvm-config --version")
    bindir = Path(bindir_result.stdout.strip())
    readobj = bindir / "llvm-readobj"
    objdump = bindir / "llvm-objdump"
    if not readobj.is_file() or not objdump.is_file():
        raise RuntimeError(f"LLVM inspection tools missing under {bindir}")
    return readobj, objdump, version_result.stdout.strip()


def version_line(command: list[str]) -> str:
    completed = run(command, cwd=FISICS_ROOT)
    require_success(completed, label=" ".join(command))
    text = completed.stdout or completed.stderr
    return text.splitlines()[0] if text.splitlines() else ""


def require_tool(name: str) -> Path:
    resolved = shutil.which(name)
    if not resolved:
        raise RuntimeError(f"required OS-P tool not found: {name}")
    # Preserve argv[0] for multi-call tools such as Homebrew LLD, whose
    # ld.lld symlink selects the ELF linker driver.
    return Path(resolved)


def run_object_case(
    case: dict[str, Any],
    *,
    compiler: Path,
    target: str,
    build_root: Path,
    readobj: Path,
    objdump: Path,
) -> dict[str, Any]:
    case_root = build_root / case["id"] / "object"
    case_root.mkdir(parents=True, exist_ok=True)
    source = lane_path(case["source"])
    object_a = case_root / "probe-a.o"
    object_b = case_root / "probe-b.o"
    command_a = [
        str(compiler),
        "--target",
        target,
        "-c",
        str(source),
        "-o",
        str(object_a),
    ]
    command_b = command_a[:-1] + [str(object_b)]
    result_a = run(command_a, cwd=FISICS_ROOT)
    require_success(result_a, label=f"{case['id']} object emission A")
    result_b = run(command_b, cwd=FISICS_ROOT)
    require_success(result_b, label=f"{case['id']} object emission B")

    if object_a.read_bytes() != object_b.read_bytes():
        raise RuntimeError(f"{case['id']} repeated object emission is not byte-identical")

    verification = verify_object(
        object_path=object_a,
        readobj=readobj,
        objdump=objdump,
        contract=case["object_contract"],
    )
    return {
        "status": "pass",
        "source": str(source.relative_to(FISICS_ROOT)),
        "source_sha256": sha256(source),
        "object_sha256": sha256(object_a),
        "repeat_object_sha256": sha256(object_b),
        "commands": [command_a, command_b],
        "verification": verification,
    }


def run_runtime_case(
    case: dict[str, Any],
    *,
    compiler: Path,
    build_root: Path,
) -> dict[str, Any]:
    case_root = build_root / case["id"] / "runtime"
    case_root.mkdir(parents=True, exist_ok=True)
    source = lane_path(case["source"])
    driver = lane_path(case["runtime_driver"])
    expected_stdout_path = lane_path(case["expected_stdout"])
    expected_stdout = expected_stdout_path.read_text(encoding="utf-8")
    expected_exit = int(case["expected_exit"])
    fisics_binary = case_root / "fisics.out"
    clang_binary = case_root / "clang.out"
    sanitizer_binary = case_root / "clang-sanitized.out"

    fisics_compile = [
        str(compiler),
        str(source),
        str(driver),
        "-o",
        str(fisics_binary),
    ]
    clang_compile = [
        "clang",
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        str(source),
        str(driver),
        "-o",
        str(clang_binary),
    ]
    sanitizer_compile = [
        "clang",
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-fsanitize=address,undefined",
        "-fno-omit-frame-pointer",
        str(source),
        str(driver),
        "-o",
        str(sanitizer_binary),
    ]
    fisics_compile_result = run(fisics_compile, cwd=FISICS_ROOT)
    require_success(fisics_compile_result, label=f"{case['id']} fisiCs runtime compile")
    clang_compile_result = run(clang_compile, cwd=FISICS_ROOT)
    require_success(clang_compile_result, label=f"{case['id']} Clang runtime compile")
    sanitizer_compile_result = run(sanitizer_compile, cwd=FISICS_ROOT)
    require_success(
        sanitizer_compile_result,
        label=f"{case['id']} Clang sanitizer compile",
    )

    fisics_run = run([str(fisics_binary)], cwd=case_root)
    clang_run = run([str(clang_binary)], cwd=case_root)
    sanitizer_run = run([str(sanitizer_binary)], cwd=case_root)
    for lane, completed in (
        ("fisiCs", fisics_run),
        ("Clang", clang_run),
        ("Clang sanitizer", sanitizer_run),
    ):
        if completed.returncode != expected_exit:
            raise RuntimeError(
                f"{case['id']} {lane} exit mismatch: "
                f"expected={expected_exit} actual={completed.returncode}"
            )
        if completed.stdout != expected_stdout:
            raise RuntimeError(
                f"{case['id']} {lane} stdout mismatch:\n"
                f"expected={expected_stdout!r}\nactual={completed.stdout!r}"
            )
        if completed.stderr:
            raise RuntimeError(f"{case['id']} {lane} emitted stderr: {completed.stderr!r}")
    if (
        fisics_run.stdout != clang_run.stdout
        or fisics_run.stdout != sanitizer_run.stdout
        or fisics_run.returncode != clang_run.returncode
        or fisics_run.returncode != sanitizer_run.returncode
    ):
        raise RuntimeError(f"{case['id']} runtime parity mismatch")

    return {
        "status": "pass",
        "source": str(source.relative_to(FISICS_ROOT)),
        "driver": str(driver.relative_to(FISICS_ROOT)),
        "expected_stdout": str(expected_stdout_path.relative_to(FISICS_ROOT)),
        "expected_exit": expected_exit,
        "commands": {
            "fisics_compile": fisics_compile,
            "clang_compile": clang_compile,
            "sanitizer_compile": sanitizer_compile,
            "fisics_run": [str(fisics_binary)],
            "clang_run": [str(clang_binary)],
            "sanitizer_run": [str(sanitizer_binary)],
        },
        "stdout_sha256": hashlib.sha256(fisics_run.stdout.encode("utf-8")).hexdigest(),
    }


def guest_compile_command(
    compiler_kind: str,
    *,
    compiler: Path,
    source: Path,
    output: Path,
    target: str,
    scalar_sse2: bool = False,
) -> list[str]:
    if compiler_kind == "fisics":
        return [
            str(compiler),
            "--target",
            target,
            "-c",
            str(source),
            "-o",
            str(output),
        ]
    if compiler_kind != "clang":
        raise RuntimeError(f"unknown guest compiler kind: {compiler_kind}")
    command = [
        "clang",
        f"--target={target}",
        "-std=c99",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-stack-protector",
        "-fno-pic",
        "-fno-pie",
        "-fno-asynchronous-unwind-tables",
        "-mno-red-zone",
        "-mno-mmx",
    ]
    command.append("-msse2" if scalar_sse2 else "-mno-sse")
    command.extend([
        "-c",
        str(source),
        "-o",
        str(output),
    ])
    return command


def build_guest_kernel(
    *,
    compiler_kind: str,
    compiler: Path,
    target: str,
    source: Path,
    adapter_object: Path,
    entry_object: Path,
    linker_script: Path,
    linker: Path,
    objcopy: Path,
    case_root: Path,
    scalar_sse2: bool,
) -> dict[str, Any]:
    variant_root = case_root / compiler_kind
    variant_root.mkdir(parents=True, exist_ok=True)
    policy_a = variant_root / "policy-a.o"
    policy_b = variant_root / "policy-b.o"
    compile_a = guest_compile_command(
        compiler_kind,
        compiler=compiler,
        source=source,
        output=policy_a,
        target=target,
        scalar_sse2=scalar_sse2,
    )
    compile_b = guest_compile_command(
        compiler_kind,
        compiler=compiler,
        source=source,
        output=policy_b,
        target=target,
        scalar_sse2=scalar_sse2,
    )
    require_success(
        run(compile_a, cwd=FISICS_ROOT),
        label=f"{source.name} {compiler_kind} guest policy compile A",
    )
    require_success(
        run(compile_b, cwd=FISICS_ROOT),
        label=f"{source.name} {compiler_kind} guest policy compile B",
    )
    if policy_a.read_bytes() != policy_b.read_bytes():
        raise RuntimeError(
            f"{source.name} {compiler_kind} guest policy object is not deterministic"
        )

    kernel_a = variant_root / "kernel-a.elf"
    kernel_b = variant_root / "kernel-b.elf"
    raw_a = variant_root / "kernel-a.bin"
    raw_b = variant_root / "kernel-b.bin"
    link_commands: list[list[str]] = []
    objcopy_commands: list[list[str]] = []
    for policy, kernel, raw in (
        (policy_a, kernel_a, raw_a),
        (policy_b, kernel_b, raw_b),
    ):
        link_command = [
            str(linker),
            "-m",
            "elf_x86_64",
            "-static",
            "--no-undefined",
            "-T",
            str(linker_script),
            f"-Map={kernel.with_suffix('.map')}",
            "-o",
            str(kernel),
            str(entry_object),
            str(adapter_object),
            str(policy),
        ]
        objcopy_command = [str(objcopy), "-O", "binary", str(kernel), str(raw)]
        require_success(
            run(link_command, cwd=FISICS_ROOT),
            label=f"{source.name} {compiler_kind} guest link",
        )
        require_success(
            run(objcopy_command, cwd=FISICS_ROOT),
            label=f"{source.name} {compiler_kind} guest objcopy",
        )
        link_commands.append(link_command)
        objcopy_commands.append(objcopy_command)
    if raw_a.read_bytes() != raw_b.read_bytes():
        raise RuntimeError(
            f"{source.name} {compiler_kind} repeated guest kernel is not deterministic"
        )

    return {
        "compiler": compiler_kind,
        "policy_object": policy_a,
        "kernel_elf": kernel_a,
        "kernel_raw": raw_a,
        "policy_object_sha256": sha256(policy_a),
        "kernel_elf_sha256": sha256(kernel_a),
        "kernel_raw_sha256": sha256(raw_a),
        "commands": {
            "compile": [compile_a, compile_b],
            "link": link_commands,
            "objcopy": objcopy_commands,
        },
    }


def run_guest_case(
    case: dict[str, Any],
    *,
    manifest: dict[str, Any],
    compiler: Path,
    build_root: Path,
) -> dict[str, Any]:
    case_root = build_root / case["id"] / "guest"
    case_root.mkdir(parents=True, exist_ok=True)
    source = lane_path(case["source"])
    guest_contract = case["guest_contract"]
    adapter_source = lane_path(guest_contract["adapter_source"])
    expected_serial_path = lane_path(guest_contract["expected_serial"])
    expected_serial = expected_serial_path.read_bytes()
    harness = manifest["guest_harness"]
    boot_source = lane_path(harness["boot_source"])
    entry_source = lane_path(harness["entry_source"])
    linker_script = lane_path(harness["linker_script"])

    nasm = require_tool("nasm")
    linker = require_tool("ld.lld")
    qemu = require_tool("qemu-system-x86_64")
    _, _, _ = llvm_tool_paths()
    llvm_bindir_result = run(["llvm-config", "--bindir"], cwd=FISICS_ROOT)
    require_success(llvm_bindir_result, label="llvm-config --bindir")
    objcopy = Path(llvm_bindir_result.stdout.strip()) / "llvm-objcopy"
    if not objcopy.is_file():
        objcopy = require_tool("llvm-objcopy")

    entry_object = case_root / "entry.o"
    adapter_object = case_root / "adapter.o"
    entry_command = [str(nasm), "-f", "elf64"]
    if guest_contract.get("scalar_sse2", False):
        entry_command.append("-DOSP_GUEST_ENABLE_SSE2=1")
    entry_command.extend([str(entry_source), "-o", str(entry_object)])
    adapter_command = guest_compile_command(
        "clang",
        compiler=compiler,
        source=adapter_source,
        output=adapter_object,
        target=manifest["target"],
        scalar_sse2=guest_contract.get("scalar_sse2", False),
    )
    require_success(
        run(entry_command, cwd=FISICS_ROOT),
        label=f"{case['id']} guest entry assemble",
    )
    require_success(
        run(adapter_command, cwd=FISICS_ROOT),
        label=f"{case['id']} guest adapter compile",
    )

    variants = [
        build_guest_kernel(
            compiler_kind=compiler_kind,
            compiler=compiler,
            target=manifest["target"],
            source=source,
            adapter_object=adapter_object,
            entry_object=entry_object,
            linker_script=linker_script,
            linker=linker,
            objcopy=objcopy,
            case_root=case_root,
            scalar_sse2=guest_contract.get("scalar_sse2", False),
        )
        for compiler_kind in ("fisics", "clang")
    ]

    sector_size = 512
    kernel_sectors = max(
        (variant["kernel_raw"].stat().st_size + sector_size - 1) // sector_size
        for variant in variants
    )
    if kernel_sectors < 1 or kernel_sectors > harness["max_kernel_sectors"]:
        raise RuntimeError(
            f"{case['id']} guest kernel requires {kernel_sectors} sectors; "
            f"allowed=1..{harness['max_kernel_sectors']}"
        )
    boot_binary = case_root / "boot.bin"
    boot_command = [
        str(nasm),
        "-f",
        "bin",
        f"-DKERNEL_SECTORS={kernel_sectors}",
        str(boot_source),
        "-o",
        str(boot_binary),
    ]
    require_success(
        run(boot_command, cwd=FISICS_ROOT),
        label=f"{case['id']} guest boot assemble",
    )
    boot_bytes = boot_binary.read_bytes()
    if len(boot_bytes) != sector_size or boot_bytes[510:512] != b"\x55\xaa":
        raise RuntimeError(f"{case['id']} guest boot sector size/signature invalid")

    run_reports: list[dict[str, Any]] = []
    serial_digests: set[str] = set()
    for variant in variants:
        raw_bytes = variant["kernel_raw"].read_bytes()
        padded_kernel = raw_bytes + (
            b"\x00" * (kernel_sectors * sector_size - len(raw_bytes))
        )
        image_path = case_root / f"{variant['compiler']}.img"
        image_path.write_bytes(boot_bytes + padded_kernel)
        variant["image"] = image_path
        variant["image_sha256"] = sha256(image_path)
        variant_runs: list[dict[str, Any]] = []
        for run_index in range(1, guest_contract["repeat_runs"] + 1):
            serial_path = (
                case_root / f"{variant['compiler']}-run-{run_index}.serial.log"
            )
            serial_path.unlink(missing_ok=True)
            qemu_command = [
                str(qemu),
                "-machine",
                harness["qemu_machine"],
                "-cpu",
                harness["qemu_cpu"],
                "-m",
                f"{harness['memory_mb']}M",
                "-smp",
                str(harness["smp"]),
                "-drive",
                (
                    "if=ide,index=0,media=disk,format=raw,"
                    f"file={image_path}"
                ),
                "-display",
                "none",
                "-serial",
                f"file:{serial_path}",
                "-monitor",
                "none",
                "-device",
                "isa-debug-exit,iobase=0xf4,iosize=0x04",
                "-no-reboot",
            ]
            started = time.time()
            try:
                completed = run(
                    qemu_command,
                    cwd=case_root,
                    timeout=guest_contract["timeout_seconds"],
                )
            except subprocess.TimeoutExpired as exc:
                serial = serial_path.read_bytes() if serial_path.exists() else b""
                raise RuntimeError(
                    f"{case['id']} {variant['compiler']} QEMU run {run_index} "
                    f"timed out; serial={serial!r}"
                ) from exc
            serial = serial_path.read_bytes() if serial_path.exists() else b""
            if completed.returncode != guest_contract["expected_exit"]:
                raise RuntimeError(
                    f"{case['id']} {variant['compiler']} QEMU exit mismatch: "
                    f"expected={guest_contract['expected_exit']} "
                    f"actual={completed.returncode}; serial={serial!r}; "
                    f"stderr={completed.stderr!r}"
                )
            if serial != expected_serial:
                raise RuntimeError(
                    f"{case['id']} {variant['compiler']} QEMU serial mismatch: "
                    f"expected={expected_serial!r} actual={serial!r}"
                )
            if completed.stdout:
                raise RuntimeError(
                    f"{case['id']} {variant['compiler']} QEMU emitted stdout: "
                    f"{completed.stdout!r}"
                )
            serial_digest = hashlib.sha256(serial).hexdigest()
            serial_digests.add(serial_digest)
            variant_runs.append(
                {
                    "run": run_index,
                    "command": qemu_command,
                    "exit": completed.returncode,
                    "duration_ms": int((time.time() - started) * 1000),
                    "serial": str(serial_path.relative_to(FISICS_ROOT)),
                    "serial_sha256": serial_digest,
                    "stderr": completed.stderr,
                }
            )
        run_reports.append(
            {
                "compiler": variant["compiler"],
                "policy_object_sha256": variant["policy_object_sha256"],
                "kernel_elf_sha256": variant["kernel_elf_sha256"],
                "kernel_raw_sha256": variant["kernel_raw_sha256"],
                "image": str(variant["image"].relative_to(FISICS_ROOT)),
                "image_sha256": variant["image_sha256"],
                "commands": variant["commands"],
                "runs": variant_runs,
            }
        )

    if len(serial_digests) != 1:
        raise RuntimeError(f"{case['id']} guest serial artifact parity mismatch")

    return {
        "status": "pass",
        "harness": {
            "boot_source": str(boot_source.relative_to(FISICS_ROOT)),
            "entry_source": str(entry_source.relative_to(FISICS_ROOT)),
            "linker_script": str(linker_script.relative_to(FISICS_ROOT)),
            "boot_command": boot_command,
            "boot_sha256": sha256(boot_binary),
            "kernel_load_address": harness["kernel_load_address"],
            "kernel_sectors": kernel_sectors,
            "machine": harness["qemu_machine"],
            "cpu": harness["qemu_cpu"],
            "memory_mb": harness["memory_mb"],
            "smp": harness["smp"],
        },
        "adapter_source": str(adapter_source.relative_to(FISICS_ROOT)),
        "adapter_command": adapter_command,
        "expected_serial": str(expected_serial_path.relative_to(FISICS_ROOT)),
        "expected_exit": guest_contract["expected_exit"],
        "debug_exit_value": guest_contract["debug_exit_value"],
        "repeat_runs": guest_contract["repeat_runs"],
        "parity_artifact": guest_contract["parity_artifact"],
        "parity_sha256": next(iter(serial_digests)),
        "variants": run_reports,
        "tools": {
            "qemu": version_line([str(qemu), "--version"]),
            "nasm": version_line([str(nasm), "-v"]),
            "linker": version_line([str(linker), "--version"]),
            "objcopy": version_line([str(objcopy), "--version"]),
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the fisiCs OS Policy lane.")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument(
        "--tier",
        choices=("object", "runtime", "guest", "all"),
        default="all",
    )
    parser.add_argument("--case", default="", help="Substring filter on case id.")
    parser.add_argument("--build-root", type=Path, default=DEFAULT_BUILD_ROOT)
    parser.add_argument(
        "--continue-on-failure",
        action="store_true",
        help="Run every selected case, record each failure, and exit nonzero.",
    )
    args = parser.parse_args()

    started = time.time()
    manifest = load_manifest(args.manifest.resolve())
    selected = select_cases(manifest, args.case)
    compiler = (FISICS_ROOT / "fisics").resolve()
    if not compiler.is_file():
        raise RuntimeError(f"fisiCs binary not found: {compiler}")
    build_root = args.build_root.resolve()
    build_root.mkdir(parents=True, exist_ok=True)
    readobj, objdump, llvm_version = llvm_tool_paths()

    report: dict[str, Any] = {
        "schema_version": 1,
        "lane_id": manifest["lane_id"],
        "lane_version": manifest["lane_version"],
        "target": manifest["target"],
        "tier": args.tier,
        "selection": {
            "kind": "full" if not args.case else "filtered",
            "canonical": not bool(args.case),
            "case_filter": args.case,
            "selected": len(selected),
            "available": len(manifest["cases"]),
        },
        "environment": {
            "platform": platform.platform(),
            "python": sys.version.split()[0],
            "llvm": llvm_version,
            "clang": version_line(["clang", "--version"]),
            "fisics": version_line([str(compiler), "--version"]),
            "fisics_sha256": sha256(compiler),
        },
        "cases": [],
    }

    cases_failed = 0
    for case in selected:
        case_report: dict[str, Any] = {
            "id": case["id"],
            "description": case["description"],
            "introduced_in": case["introduced_in"],
            "provenance": case["provenance"],
        }
        try:
            if args.tier in ("object", "all"):
                case_report["object"] = run_object_case(
                    case,
                    compiler=compiler,
                    target=manifest["target"],
                    build_root=build_root,
                    readobj=readobj,
                    objdump=objdump,
                )
            if args.tier in ("runtime", "all"):
                case_report["runtime"] = run_runtime_case(
                    case,
                    compiler=compiler,
                    build_root=build_root,
                )
            if args.tier in ("guest", "all"):
                case_report["guest"] = run_guest_case(
                    case,
                    manifest=manifest,
                    compiler=compiler,
                    build_root=build_root,
                )
        except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
            if not args.continue_on_failure:
                raise
            cases_failed += 1
            case_report["status"] = "fail"
            case_report["error"] = str(exc)
            report["cases"].append(case_report)
            print(
                f"FAIL {case['id']} tier={args.tier}: {exc}",
                file=sys.stderr,
            )
            continue
        case_report["status"] = "pass"
        report["cases"].append(case_report)
        print(f"PASS {case['id']} tier={args.tier}")

    cases_passed = len(report["cases"]) - cases_failed
    report["summary"] = {
        "status": "fail" if cases_failed else "pass",
        "cases_passed": cases_passed,
        "cases_failed": cases_failed,
        "duration_ms": int((time.time() - started) * 1000),
    }
    report_path = build_root / f"latest-{args.tier}.json"
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(
        f"OS-P {manifest['lane_version']} "
        f"{'FAIL' if cases_failed else 'PASS'} tier={args.tier} "
        f"cases={len(report['cases'])} passed={cases_passed} "
        f"failed={cases_failed} canonical={int(not bool(args.case))}"
    )
    print(f"report={report_path}")
    return 1 if cases_failed else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.SubprocessError) as exc:
        print(f"OS-P FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
