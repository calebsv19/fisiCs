#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from failure_taxonomy import (
    classify_real_project_blocker,
    format_real_project_blocker_line,
)


SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
FISICS_ROOT = REAL_PROJECTS_ROOT.parent.parent
WORKSPACE_ROOT = FISICS_ROOT.parent
DEFAULT_MANIFEST = REAL_PROJECTS_ROOT / "config" / "projects_manifest.json"
DEFAULT_REPORT_ROOT = REAL_PROJECTS_ROOT / "reports"
DEFAULT_ARTIFACT_ROOT = REAL_PROJECTS_ROOT / "artifacts"
DEFAULT_STAGE_KEY = "D_runtime_smoke"


@dataclass
class CommandResult:
    ok: bool
    returncode: int | None
    timed_out: bool
    duration_ms: int
    stdout: str
    stderr: str
    signal: int | None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run real-project runtime smoke validation (Stage D)."
    )
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--project", default=None, help="Project name in manifest.")
    parser.add_argument("--stage", default=DEFAULT_STAGE_KEY, choices=[DEFAULT_STAGE_KEY])
    parser.add_argument("--target", default="", help="Substring filter on target id.")
    parser.add_argument("--limit", type=int, default=0, help="Limit number of runtime-smoke targets.")
    parser.add_argument("--timeout-sec", type=int, default=0, help="Override compile/link timeout.")
    parser.add_argument("--run-timeout-sec", type=int, default=0, help="Override runtime timeout.")
    parser.add_argument("--skip-clang", action="store_true", help="Run only fisics lane.")
    parser.add_argument("--dry-run", action="store_true", help="Print commands only.")
    return parser.parse_args()


def load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)
    if "projects" not in data or not isinstance(data["projects"], list):
        raise ValueError(f"invalid manifest (missing projects list): {path}")
    return data


def pick_project(manifest: dict[str, Any], project_name: str | None) -> dict[str, Any]:
    target = project_name or manifest.get("default_project")
    if not target:
        raise ValueError("manifest missing default_project and no --project provided")
    for project in manifest["projects"]:
        if project.get("name") == target:
            return project
    raise ValueError(f"project not found in manifest: {target}")


def resolve_project_root(project_root_value: str) -> Path:
    raw = Path(project_root_value)
    if raw.is_absolute():
        return raw
    repo_relative = (FISICS_ROOT / raw).resolve()
    if repo_relative.exists():
        return repo_relative
    return (WORKSPACE_ROOT / raw).resolve()


def merged_project_lists(project: dict[str, Any], key: str) -> list[str]:
    values: list[str] = list(project.get(key, []))
    os_key = platform.system().lower()
    overrides = project.get("platform_overrides", {}).get(os_key, {})
    values.extend(overrides.get(key, []))
    return values


def render_preprocessor_flags(
    project_root: Path, include_dirs: list[str], defines: list[str]
) -> list[str]:
    flags: list[str] = []
    for include_dir in include_dirs:
        include_path = Path(include_dir)
        if not include_path.is_absolute():
            include_path = (project_root / include_path).resolve()
        flags.append(f"-I{include_path}")
    for define in defines:
        flags.append(f"-D{define}")
    return flags


def run_cmd(
    cmd: list[str],
    cwd: Path,
    timeout_sec: int,
    dry_run: bool,
    env: dict[str, str] | None = None,
) -> CommandResult:
    if dry_run:
        return CommandResult(
            ok=True,
            returncode=0,
            timed_out=False,
            duration_ms=0,
            stdout="",
            stderr="",
            signal=None,
        )

    t0 = time.perf_counter()
    try:
        completed = subprocess.run(
            cmd,
            cwd=str(cwd),
            text=True,
            capture_output=True,
            timeout=timeout_sec,
            check=False,
            env=env,
        )
        elapsed_ms = int((time.perf_counter() - t0) * 1000.0)
        signal = -completed.returncode if completed.returncode < 0 else None
        return CommandResult(
            ok=(completed.returncode == 0),
            returncode=completed.returncode,
            timed_out=False,
            duration_ms=elapsed_ms,
            stdout=completed.stdout,
            stderr=completed.stderr,
            signal=signal,
        )
    except subprocess.TimeoutExpired as exc:
        elapsed_ms = int((time.perf_counter() - t0) * 1000.0)
        stdout = exc.stdout if isinstance(exc.stdout, str) else ""
        stderr = exc.stderr if isinstance(exc.stderr, str) else ""
        return CommandResult(
            ok=False,
            returncode=None,
            timed_out=True,
            duration_ms=elapsed_ms,
            stdout=stdout,
            stderr=stderr,
            signal=None,
        )


def classify_failure(result: CommandResult) -> str:
    if result.timed_out:
        return "timeout"
    if result.signal is not None:
        return "codegen"
    text = (result.stdout + "\n" + result.stderr).lower()

    parser_markers = (
        "parse error",
        "parser",
        "unexpected token",
        "expected",
        "unterminated",
        "invalid preprocessing directive",
    )
    semantic_markers = (
        "incompatible",
        "undeclared",
        "redefinition",
        "invalid operands",
        "cannot",
        "not assignable",
        "type mismatch",
        "semantic analysis",
        "semantic errors",
    )
    codegen_markers = (
        "llvm",
        "codegen",
        "backend",
        "failed to emit",
        "fatal error",
        "instruction",
    )
    link_markers = (
        "undefined reference",
        "ld:",
        "linker",
        "duplicate symbol",
    )

    if any(marker in text for marker in link_markers):
        return "link"
    if any(marker in text for marker in parser_markers):
        return "parser"
    if any(marker in text for marker in semantic_markers):
        return "semantic"
    if any(marker in text for marker in codegen_markers):
        return "codegen"
    return "semantic"


def parity_label(fisics_ok: bool, clang_ok: bool | None) -> str:
    if clang_ok is None:
        return "fisics_only"
    if fisics_ok and clang_ok:
        return "both_pass"
    if (not fisics_ok) and clang_ok:
        return "fisics_only_fail"
    if fisics_ok and (not clang_ok):
        return "clang_only_fail"
    return "both_fail"


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        f.write(value)


def resolve_input_path(project_root: Path, raw: str) -> Path:
    p = Path(raw)
    if p.is_absolute():
        return p.resolve()
    return (project_root / p).resolve()


def resolve_working_dir(project_root: Path, raw: str | None) -> Path:
    if not raw:
        return project_root
    p = Path(raw)
    if p.is_absolute():
        return p.resolve()
    return (project_root / p).resolve()


def merge_run_env(raw_env: dict[str, Any] | None) -> dict[str, str] | None:
    if not raw_env:
        return None
    # Keep the implementation explicit: inherit process env and overlay target-specific keys.
    merged = dict(os.environ)
    for key, value in raw_env.items():
        merged[str(key)] = str(value)
    return merged


def index_targets_by_id(stage_cfg: dict[str, Any]) -> dict[str, dict[str, Any]]:
    out: dict[str, dict[str, Any]] = {}
    for target in list(stage_cfg.get("targets", [])):
        target_id = str(target.get("id", "")).strip()
        if target_id:
            out[target_id] = target
    return out


def normalize_name(path: Path) -> str:
    return path.as_posix().replace("/", "__")


def source_label(project_root: Path, source: Path) -> str:
    try:
        return source.relative_to(project_root).as_posix()
    except ValueError:
        return f"__external__/{source.name}"


def source_key(project_root: Path, source: Path) -> str:
    try:
        return source.relative_to(project_root).as_posix()
    except ValueError:
        return "__external__/" + source.as_posix().lstrip("/").replace("/", "__")


def compile_lane(
    *,
    compiler_cmd: list[str],
    input_paths: list[Path],
    output_root: Path,
    project_root: Path,
    timeout_sec: int,
    dry_run: bool,
) -> tuple[list[Path], list[dict[str, Any]], bool, int]:
    objects: list[Path] = []
    rows: list[dict[str, Any]] = []
    ok = True
    total_ms = 0

    for source in input_paths:
        obj = output_root / f"{normalize_name(Path(source_key(project_root, source)))}.o"
        obj.parent.mkdir(parents=True, exist_ok=True)
        cmd = compiler_cmd + ["-c", str(source), "-o", str(obj)]
        result = run_cmd(cmd=cmd, cwd=project_root, timeout_sec=timeout_sec, dry_run=dry_run)
        total_ms += result.duration_ms
        rows.append(
            {
                "source": source_label(project_root, source),
                "cmd": cmd,
                "result": result,
            }
        )
        if result.ok:
            objects.append(obj)
        else:
            ok = False
            break
    return objects, rows, ok, total_ms


def save_target_artifacts(
    *,
    stage_dir: Path,
    target_id: str,
    lane_name: str,
    compile_rows: list[dict[str, Any]],
    link_cmd: list[str] | None,
    link_result: CommandResult | None,
    run_cmdline: list[str] | None,
    run_result: CommandResult | None,
) -> None:
    target_dir = stage_dir / target_id / lane_name
    target_dir.mkdir(parents=True, exist_ok=True)

    for row in compile_rows:
        source_rel = row["source"]
        suffix_base = normalize_name(Path(source_rel))
        result: CommandResult = row["result"]
        write_text(target_dir / f"{suffix_base}.cmd.txt", " ".join(row["cmd"]) + "\n")
        write_text(target_dir / f"{suffix_base}.stdout.txt", result.stdout)
        write_text(target_dir / f"{suffix_base}.stderr.txt", result.stderr)

    if link_cmd is not None and link_result is not None:
        write_text(target_dir / "link.cmd.txt", " ".join(link_cmd) + "\n")
        write_text(target_dir / "link.stdout.txt", link_result.stdout)
        write_text(target_dir / "link.stderr.txt", link_result.stderr)
    if run_cmdline is not None and run_result is not None:
        write_text(target_dir / "run.cmd.txt", " ".join(run_cmdline) + "\n")
        write_text(target_dir / "run.stdout.txt", run_result.stdout)
        write_text(target_dir / "run.stderr.txt", run_result.stderr)


def run_stage_d(
    project: dict[str, Any],
    project_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
    if not stage_cfg.get("enabled", False):
        raise RuntimeError(f"stage not enabled in manifest: {DEFAULT_STAGE_KEY}")
    compile_timeout_sec = args.timeout_sec or int(stage_cfg.get("timeout_sec", 120))
    run_timeout_sec = args.run_timeout_sec or int(stage_cfg.get("run_timeout_sec", 10))
    default_expected_exit_code = int(stage_cfg.get("expected_exit_code", 0))
    default_fisics_extra_args = list(stage_cfg.get("fisics_extra_args", []))
    default_clang_extra_args = list(stage_cfg.get("clang_extra_args", []))
    default_link_args = list(stage_cfg.get("link_args", []))
    default_link_inputs = list(stage_cfg.get("link_inputs", []))
    default_run_args = list(stage_cfg.get("run_args", []))
    default_run_env = dict(stage_cfg.get("run_env", {}))
    default_run_cwd = stage_cfg.get("run_cwd")
    stage_c_cfg = project.get("stages", {}).get("C_full_build", {})
    stage_c_targets = index_targets_by_id(stage_c_cfg)

    include_dirs = merged_project_lists(project, "include_dirs")
    defines = merged_project_lists(project, "defines")
    pp_flags = render_preprocessor_flags(project_root, include_dirs, defines)

    targets = list(stage_cfg.get("targets", []))
    if not targets:
        raise RuntimeError("stage D has no targets configured in manifest")
    if args.target:
        targets = [t for t in targets if args.target in str(t.get("id", ""))]
    if args.limit > 0:
        targets = targets[: args.limit]
    if not targets:
        raise RuntimeError("no stage D targets selected")

    latest_stage_dir = DEFAULT_ARTIFACT_ROOT / "latest" / project["name"] / DEFAULT_STAGE_KEY
    history_id = f"{time.strftime('%Y%m%dT%H%M%S')}_{time.time_ns() % 1_000_000_000:09d}"
    history_stage_dir = (
        DEFAULT_ARTIFACT_ROOT / "history" / history_id / project["name"] / DEFAULT_STAGE_KEY
    )
    latest_stage_dir.mkdir(parents=True, exist_ok=True)

    fisics_bin = (FISICS_ROOT / "fisics").resolve()
    if not fisics_bin.exists():
        raise RuntimeError(f"fisics binary not found: {fisics_bin}")

    results: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="fisics-realproj-staged-") as tmp:
        tmp_root = Path(tmp)
        for target in targets:
            target_id = str(target.get("id", "")).strip()
            if not target_id:
                raise RuntimeError("stage D target missing id")

            build_target_id = str(target.get("build_target", "")).strip()
            build_target = stage_c_targets.get(build_target_id) if build_target_id else None

            raw_inputs = list(target.get("inputs", []))
            if not raw_inputs and build_target is not None:
                raw_inputs = list(build_target.get("inputs", []))
            if not raw_inputs:
                raise RuntimeError(f"stage D target '{target_id}' missing inputs")
            input_paths = [resolve_input_path(project_root, raw) for raw in raw_inputs]
            for p in input_paths:
                if not p.exists() or not p.is_file():
                    raise RuntimeError(f"stage D target '{target_id}' input missing: {p}")

            target_fisics_args = (
                default_fisics_extra_args
                + (list(build_target.get("fisics_extra_args", [])) if build_target is not None else [])
                + list(target.get("fisics_extra_args", []))
            )
            target_clang_args = (
                default_clang_extra_args
                + (list(build_target.get("clang_extra_args", [])) if build_target is not None else [])
                + list(target.get("clang_extra_args", []))
            )
            target_link_args = (
                default_link_args
                + (list(build_target.get("link_args", [])) if build_target is not None else [])
                + list(target.get("link_args", []))
            )
            raw_link_inputs = (
                default_link_inputs
                + (list(build_target.get("link_inputs", [])) if build_target is not None else [])
                + list(target.get("link_inputs", []))
            )
            link_inputs = [resolve_input_path(project_root, raw) for raw in raw_link_inputs]
            for p in link_inputs:
                if not p.exists():
                    raise RuntimeError(f"stage D target '{target_id}' link input missing: {p}")

            run_args = default_run_args + list(target.get("run_args", []))
            run_env_raw = dict(default_run_env)
            run_env_raw.update(dict(target.get("run_env", {})))
            run_env = merge_run_env(run_env_raw)
            run_cwd_value_any = target.get("run_cwd", default_run_cwd)
            run_cwd_value = None if run_cwd_value_any is None else str(run_cwd_value_any)
            run_cwd = resolve_working_dir(project_root, run_cwd_value)
            if not run_cwd.exists() or not run_cwd.is_dir():
                raise RuntimeError(f"stage D target '{target_id}' run cwd missing: {run_cwd}")
            expected_exit_code = int(target.get("expected_exit_code", default_expected_exit_code))

            fisics_compile_cmd = [str(fisics_bin)] + pp_flags + target_fisics_args
            fisics_obj_root = tmp_root / "fisics" / target_id
            fisics_objects, fisics_compile_rows, fisics_compile_ok, fisics_compile_ms = compile_lane(
                compiler_cmd=fisics_compile_cmd,
                input_paths=input_paths,
                output_root=fisics_obj_root,
                project_root=project_root,
                timeout_sec=compile_timeout_sec,
                dry_run=args.dry_run,
            )

            fisics_link_cmd: list[str] | None = None
            fisics_link_result: CommandResult | None = None
            fisics_run_cmd: list[str] | None = None
            fisics_run_result: CommandResult | None = None
            fisics_link_ok = False
            fisics_run_ok = False
            fisics_link_ms = 0
            fisics_run_ms = 0
            fisics_bin_out: Path | None = None
            if fisics_compile_ok:
                fisics_bin_out = tmp_root / "fisics" / f"{target_id}.out"
                fisics_link_cmd = (
                    ["clang"]
                    + [str(obj) for obj in fisics_objects]
                    + [str(path) for path in link_inputs]
                    + target_link_args
                    + ["-o", str(fisics_bin_out)]
                )
                fisics_link_result = run_cmd(
                    cmd=fisics_link_cmd,
                    cwd=project_root,
                    timeout_sec=compile_timeout_sec,
                    dry_run=args.dry_run,
                )
                fisics_link_ok = fisics_link_result.ok
                fisics_link_ms = fisics_link_result.duration_ms
                if fisics_link_ok:
                    fisics_run_cmd = [str(fisics_bin_out)] + run_args
                    fisics_run_result = run_cmd(
                        cmd=fisics_run_cmd,
                        cwd=run_cwd,
                        timeout_sec=run_timeout_sec,
                        dry_run=args.dry_run,
                        env=run_env,
                    )
                    fisics_run_ms = fisics_run_result.duration_ms
                    fisics_run_ok = (
                        not fisics_run_result.timed_out
                        and fisics_run_result.signal is None
                        and fisics_run_result.returncode == expected_exit_code
                    )

            clang_compile_rows: list[dict[str, Any]] = []
            clang_link_cmd: list[str] | None = None
            clang_link_result: CommandResult | None = None
            clang_run_cmd: list[str] | None = None
            clang_run_result: CommandResult | None = None
            clang_compile_ok: bool | None = None
            clang_link_ok: bool | None = None
            clang_run_ok: bool | None = None
            clang_compile_ms = 0
            clang_link_ms = 0
            clang_run_ms = 0

            if not args.skip_clang:
                clang_compile_cmd = ["clang"] + target_clang_args + pp_flags
                clang_obj_root = tmp_root / "clang" / target_id
                clang_objects, clang_compile_rows, clang_compile_ok, clang_compile_ms = compile_lane(
                    compiler_cmd=clang_compile_cmd,
                    input_paths=input_paths,
                    output_root=clang_obj_root,
                    project_root=project_root,
                    timeout_sec=compile_timeout_sec,
                    dry_run=args.dry_run,
                )
                if clang_compile_ok:
                    clang_bin_out = tmp_root / "clang" / f"{target_id}.out"
                    clang_link_cmd = (
                        ["clang"]
                        + [str(obj) for obj in clang_objects]
                        + [str(path) for path in link_inputs]
                        + target_link_args
                        + ["-o", str(clang_bin_out)]
                    )
                    clang_link_result = run_cmd(
                        cmd=clang_link_cmd,
                        cwd=project_root,
                        timeout_sec=compile_timeout_sec,
                        dry_run=args.dry_run,
                    )
                    clang_link_ok = clang_link_result.ok
                    clang_link_ms = clang_link_result.duration_ms
                    if clang_link_ok:
                        clang_run_cmd = [str(clang_bin_out)] + run_args
                        clang_run_result = run_cmd(
                            cmd=clang_run_cmd,
                            cwd=run_cwd,
                            timeout_sec=run_timeout_sec,
                            dry_run=args.dry_run,
                            env=run_env,
                        )
                        clang_run_ms = clang_run_result.duration_ms
                        clang_run_ok = (
                            not clang_run_result.timed_out
                            and clang_run_result.signal is None
                            and clang_run_result.returncode == expected_exit_code
                        )
                    else:
                        clang_run_ok = False
                else:
                    clang_link_ok = False
                    clang_run_ok = False

            save_target_artifacts(
                stage_dir=latest_stage_dir,
                target_id=target_id,
                lane_name="fisics",
                compile_rows=fisics_compile_rows,
                link_cmd=fisics_link_cmd,
                link_result=fisics_link_result,
                run_cmdline=fisics_run_cmd,
                run_result=fisics_run_result,
            )
            if not args.skip_clang:
                save_target_artifacts(
                    stage_dir=latest_stage_dir,
                    target_id=target_id,
                    lane_name="clang",
                    compile_rows=clang_compile_rows,
                    link_cmd=clang_link_cmd,
                    link_result=clang_link_result,
                    run_cmdline=clang_run_cmd,
                    run_result=clang_run_result,
                )

            fisics_ok = fisics_compile_ok and fisics_link_ok and fisics_run_ok
            clang_ok = None
            if not args.skip_clang:
                clang_ok = bool(clang_compile_ok and clang_link_ok and clang_run_ok)
            parity = parity_label(fisics_ok=fisics_ok, clang_ok=clang_ok)
            is_blocker = (not fisics_ok) and (clang_ok is None or clang_ok)

            failure_class = None
            failure_phase = None
            if not fisics_compile_ok:
                first_fail = next(
                    (row["result"] for row in fisics_compile_rows if not row["result"].ok),
                    None,
                )
                if first_fail is not None:
                    failure_class = classify_failure(first_fail)
                    failure_phase = "compile"
            elif fisics_link_result is not None and not fisics_link_result.ok:
                failure_class = "link"
                failure_phase = "link"
            elif fisics_run_result is not None and not fisics_run_ok:
                failure_phase = "runtime"
                if fisics_run_result.timed_out:
                    failure_class = "timeout"
                elif fisics_run_result.signal is not None:
                    failure_class = "runtime_crash"
                elif fisics_run_result.returncode != expected_exit_code:
                    failure_class = "runtime"
            blocker_classification = None
            if is_blocker:
                blocker_classification = classify_real_project_blocker(
                    DEFAULT_STAGE_KEY,
                    {
                        "target": target_id,
                        "parity": parity,
                        "fisics": {
                            "failure_phase": failure_phase,
                            "failure_class": failure_class,
                        },
                    },
                )

            results.append(
                {
                    "target": target_id,
                    "build_target": build_target_id if build_target_id else None,
                    "inputs": [source_label(project_root, p) for p in input_paths],
                    "link_inputs": [source_label(project_root, p) for p in link_inputs],
                    "run": {
                        "args": run_args,
                        "cwd": str(run_cwd),
                        "expected_exit_code": expected_exit_code,
                    },
                    "parity": parity,
                    "is_blocker": is_blocker,
                    "blocker_classification": blocker_classification,
                    "fisics": {
                        "ok": fisics_ok,
                        "compile_ok": fisics_compile_ok,
                        "link_ok": fisics_link_ok,
                        "run_ok": fisics_run_ok,
                        "failure_phase": failure_phase,
                        "failure_class": failure_class,
                        "timing_ms": {
                            "compile_total": fisics_compile_ms,
                            "link": fisics_link_ms,
                            "run": fisics_run_ms,
                            "total": fisics_compile_ms + fisics_link_ms + fisics_run_ms,
                        },
                        "run_result": None
                        if fisics_run_result is None
                        else {
                            "returncode": fisics_run_result.returncode,
                            "timed_out": fisics_run_result.timed_out,
                            "signal": fisics_run_result.signal,
                        },
                    },
                    "clang": None
                    if args.skip_clang
                    else {
                        "ok": bool(clang_compile_ok and clang_link_ok and clang_run_ok),
                        "compile_ok": bool(clang_compile_ok),
                        "link_ok": bool(clang_link_ok),
                        "run_ok": bool(clang_run_ok),
                        "timing_ms": {
                            "compile_total": clang_compile_ms,
                            "link": clang_link_ms,
                            "run": clang_run_ms,
                            "total": clang_compile_ms + clang_link_ms + clang_run_ms,
                        },
                        "run_result": None
                        if clang_run_result is None
                        else {
                            "returncode": clang_run_result.returncode,
                            "timed_out": clang_run_result.timed_out,
                            "signal": clang_run_result.signal,
                        },
                    },
                }
            )

    if latest_stage_dir.exists():
        history_stage_dir.parent.mkdir(parents=True, exist_ok=True)
        if history_stage_dir.exists():
            shutil.rmtree(history_stage_dir)
        shutil.copytree(latest_stage_dir, history_stage_dir)

    parity_counts: dict[str, int] = {}
    blocker_count = 0
    fisics_total_ms = 0
    clang_total_ms = 0
    for row in results:
        parity_counts[row["parity"]] = parity_counts.get(row["parity"], 0) + 1
        if row["is_blocker"]:
            blocker_count += 1
        fisics_total_ms += int(row["fisics"]["timing_ms"]["total"])
        if row["clang"] is not None:
            clang_total_ms += int(row["clang"]["timing_ms"]["total"])

    report = {
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "history_id": history_id,
        "workspace_root": str(WORKSPACE_ROOT),
        "fisics_root": str(FISICS_ROOT),
        "project": {
            "name": project["name"],
            "root": str(project_root),
            "entry": project.get("entry"),
            "kind": project.get("kind"),
        },
        "stage": DEFAULT_STAGE_KEY,
        "dry_run": args.dry_run,
        "targets_total": len(results),
        "timeout_sec": compile_timeout_sec,
        "run_timeout_sec": run_timeout_sec,
        "summary": {
            "parity_counts": parity_counts,
            "blockers": blocker_count,
            "fisics_total_ms": fisics_total_ms,
            "clang_total_ms": clang_total_ms,
        },
        "results": results,
    }
    return report


def write_report(report: dict[str, Any]) -> tuple[Path, Path]:
    project_name = report["project"]["name"]
    history_id = report["history_id"]
    latest_dir = DEFAULT_REPORT_ROOT / "latest"
    history_dir = DEFAULT_REPORT_ROOT / "history"
    latest_dir.mkdir(parents=True, exist_ok=True)
    history_dir.mkdir(parents=True, exist_ok=True)

    latest_path = latest_dir / f"{project_name}_{DEFAULT_STAGE_KEY}_latest.json"
    history_path = history_dir / f"{history_id}_{project_name}_{DEFAULT_STAGE_KEY}.json"
    with latest_path.open("w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)
        f.write("\n")
    with history_path.open("w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)
        f.write("\n")
    return latest_path, history_path


def print_summary(report: dict[str, Any], latest_path: Path, history_path: Path) -> None:
    summary = report["summary"]
    parity_counts = summary["parity_counts"]
    print(f"project={report['project']['name']} stage={report['stage']} targets={report['targets_total']}")
    print(f"blockers={summary['blockers']} parity={json.dumps(parity_counts, sort_keys=True)}")
    for row in report["results"]:
        if row["is_blocker"]:
            print(format_real_project_blocker_line(report["stage"], row))
    print(f"timing_ms fisics={summary['fisics_total_ms']} clang={summary['clang_total_ms']}")
    print(f"latest_report={latest_path}")
    print(f"history_report={history_path}")
    print(
        "latest_artifacts="
        f"{DEFAULT_ARTIFACT_ROOT / 'latest' / report['project']['name'] / DEFAULT_STAGE_KEY}"
    )


def main() -> int:
    args = parse_args()
    try:
        manifest = load_manifest(args.manifest.resolve())
        project = pick_project(manifest, args.project)
        project_root = resolve_project_root(str(project["root"]))
        if not project_root.exists():
            raise RuntimeError(f"project root does not exist: {project_root}")
        report = run_stage_d(project=project, project_root=project_root, args=args)
        latest_path, history_path = write_report(report)
        print_summary(report, latest_path=latest_path, history_path=history_path)
        return 2 if report["summary"]["blockers"] > 0 else 0
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
