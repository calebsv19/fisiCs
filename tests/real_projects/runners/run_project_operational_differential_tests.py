#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shlex
import shutil
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Any

from report_contract import (
    build_report_contract,
    canonical_stage_closure_enabled,
    git_meta,
    history_artifact_dir,
    latest_artifact_dir,
    write_json_report,
)

SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
FISICS_ROOT = REAL_PROJECTS_ROOT.parent.parent
WORKSPACE_ROOT = FISICS_ROOT.parent
MANIFEST = REAL_PROJECTS_ROOT / "config" / "projects_manifest.json"
REPORT_ROOT = REAL_PROJECTS_ROOT / "reports"
ARTIFACT_ROOT = REAL_PROJECTS_ROOT / "artifacts"
STAGE = "G_operational_differential"


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser(description="Run Stage-G semantic operational differentials.")
    p.add_argument("--project", required=True)
    p.add_argument("--target", default="")
    p.add_argument("--repeat", type=int, default=2)
    p.add_argument("--timeout-sec", type=int, default=0)
    p.add_argument("--dry-run", action="store_true")
    return p.parse_args()


def resolve_root(value: str) -> Path:
    path = Path(value)
    if path.is_absolute():
        return path
    repo_path = (FISICS_ROOT / path).resolve()
    return repo_path if repo_path.exists() else (WORKSPACE_ROOT / path).resolve()


def merged(project: dict[str, Any], key: str) -> list[str]:
    values = list(project.get(key, []))
    values += project.get("platform_overrides", {}).get(platform.system().lower(), {}).get(key, [])
    return values


def resolve_input(project_root: Path, value: str) -> Path:
    raw = Path(value)
    if raw.is_absolute():
        return raw
    candidate = (project_root / raw).resolve()
    if candidate.exists():
        return candidate
    candidate = (FISICS_ROOT / raw).resolve()
    if candidate.exists():
        return candidate
    return (WORKSPACE_ROOT / raw).resolve()


def run(
    cmd: list[str],
    cwd: Path,
    timeout: int,
    dry_run: bool = False,
    env: dict[str, str] | None = None,
) -> dict[str, Any]:
    started = time.perf_counter()
    if dry_run:
        return {"ok": True, "returncode": 0, "timed_out": False, "duration_ms": 0, "stdout": "", "stderr": ""}
    try:
        cp = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            text=True,
            capture_output=True,
            timeout=timeout,
            check=False,
        )
        return {
            "ok": cp.returncode == 0,
            "returncode": cp.returncode,
            "timed_out": False,
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "stdout": cp.stdout,
            "stderr": cp.stderr,
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "ok": False,
            "returncode": None,
            "timed_out": True,
            "duration_ms": int((time.perf_counter() - started) * 1000),
            "stdout": exc.stdout if isinstance(exc.stdout, str) else "",
            "stderr": exc.stderr if isinstance(exc.stderr, str) else "",
        }


def save_command(path: Path, cmd: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(shlex.join(cmd) + "\n", encoding="utf-8")


def save_result(root: Path, name: str, cmd: list[str], result: dict[str, Any]) -> None:
    save_command(root / f"{name}.cmd.txt", cmd)
    (root / f"{name}.stdout.txt").write_text(result["stdout"], encoding="utf-8")
    (root / f"{name}.stderr.txt").write_text(result["stderr"], encoding="utf-8")


def trace_contract(stdout: str, expected: list[str]) -> tuple[bool, list[str], str]:
    lines = stdout.splitlines()
    trace_lines = [line for line in lines if line.startswith("TRACE|")]
    if len(trace_lines) != len(lines):
        return False, trace_lines, "stdout contains non-trace output"
    names: list[str] = []
    for line in trace_lines:
        parts = line.split("|")
        if len(parts) < 6 or parts[0] != "TRACE" or parts[1] != "1":
            return False, trace_lines, "malformed trace line"
        names.append(parts[2])
    if len(names) != len(set(names)):
        return False, trace_lines, "duplicate checkpoints"
    if names != expected:
        return False, trace_lines, f"checkpoint order mismatch expected={expected} actual={names}"
    return True, trace_lines, ""


def select_targets(
    targets: list[dict[str, Any]], target_id: str
) -> tuple[list[dict[str, Any]], str]:
    selected = [target for target in targets if target.get("id") == target_id] if target_id else list(targets)
    if not selected:
        raise ValueError("zero Stage-G targets selected")
    selection_kind = "filtered" if target_id or len(selected) != len(targets) else "full"
    return selected, selection_kind


def _safe_fixture_destination(value: str) -> Path:
    destination = Path(value)
    if not value or destination.is_absolute() or ".." in destination.parts:
        raise ValueError(f"unsafe runtime fixture destination: {value!r}")
    normalized = Path(*[part for part in destination.parts if part not in ("", ".")])
    if not normalized.parts:
        raise ValueError(f"unsafe runtime fixture destination: {value!r}")
    return normalized


def _file_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stage_runtime_fixtures(
    project_root: Path,
    fixtures: list[dict[str, Any]],
    run_root: Path,
) -> tuple[list[dict[str, Any]], set[str]]:
    destinations: set[str] = set()
    provenance: list[dict[str, Any]] = []
    staged_files: set[str] = set()
    for fixture in fixtures:
        source_value = str(fixture.get("source", ""))
        destination_value = str(fixture.get("path", ""))
        destination_relative = _safe_fixture_destination(destination_value)
        destination_key = destination_relative.as_posix()
        if destination_key in destinations:
            raise ValueError(f"duplicate runtime fixture destination: {destination_key}")
        destinations.add(destination_key)
        source = resolve_input(project_root, source_value)
        if not source.exists():
            raise FileNotFoundError(f"runtime fixture missing: {source_value}")
        destination = run_root / destination_relative
        if source.is_dir():
            shutil.copytree(source, destination)
            files = sorted(path for path in destination.rglob("*") if path.is_file())
        elif source.is_file():
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            files = [destination]
        else:
            raise ValueError(f"runtime fixture is not a regular file or directory: {source}")
        file_records = []
        for staged in files:
            relative = staged.relative_to(run_root).as_posix()
            staged_files.add(relative)
            file_records.append({"path": relative, "sha256_before": _file_sha256(staged)})
        provenance.append(
            {
                "source": str(source),
                "destination": destination_key,
                "files": file_records,
            }
        )
    return provenance, staged_files


def build_run_environment(
    inherited: dict[str, str],
    scrub_prefixes: list[str],
    configured: dict[str, Any],
    project_root: Path,
    run_root: Path,
) -> tuple[dict[str, str], dict[str, str]]:
    for prefix in scrub_prefixes:
        if not prefix:
            raise ValueError("runtime environment scrub prefix must not be empty")
    environment = {
        key: value
        for key, value in inherited.items()
        if not any(key.startswith(prefix) for prefix in scrub_prefixes)
    }
    placeholders = {
        "project_root": str(project_root),
        "run_root": str(run_root),
        "fixture_root": str(run_root),
    }
    applied: dict[str, str] = {}
    for key, raw_value in configured.items():
        try:
            value = str(raw_value).format(**placeholders)
        except KeyError as exc:
            raise ValueError(f"unknown run_env placeholder for {key}: {exc.args[0]}") from exc
        environment[str(key)] = value
        applied[str(key)] = value
    return environment, applied


def finalize_fixture_provenance(
    run_root: Path, provenance: list[dict[str, Any]]
) -> list[dict[str, Any]]:
    finalized = json.loads(json.dumps(provenance))
    for fixture in finalized:
        for file_record in fixture["files"]:
            staged = run_root / file_record["path"]
            file_record["exists_after"] = staged.is_file()
            file_record["sha256_after"] = _file_sha256(staged) if staged.is_file() else ""
            file_record["mutated"] = file_record["sha256_after"] != file_record["sha256_before"]
    return finalized


def compile_lane(
    lane: str,
    compiler: Path | str,
    project_root: Path,
    target: dict[str, Any],
    preprocessor: list[str],
    stage_cfg: dict[str, Any],
    build_root: Path,
    artifact_root: Path,
    timeout: int,
    dry_run: bool,
) -> dict[str, Any]:
    objects: list[Path] = []
    results: list[dict[str, Any]] = []
    extras = stage_cfg.get("fisics_extra_args" if lane == "fisics" else "clang_extra_args", [])
    for index, value in enumerate(target.get("inputs", [])):
        source = resolve_input(project_root, value)
        obj = build_root / lane / f"{index:02d}_{source.stem}.o"
        obj.parent.mkdir(parents=True, exist_ok=True)
        cmd = [str(compiler), *preprocessor, *extras, "-c", str(source), "-o", str(obj)]
        result = run(cmd, project_root, timeout, dry_run)
        save_result(artifact_root / lane, f"compile_{index:02d}_{source.stem}", cmd, result)
        results.append({"source": str(source), "command": cmd, **result})
        if not result["ok"]:
            return {"ok": False, "phase": "compile", "commands": results, "executable": None}
        objects.append(obj)
    executable = build_root / lane / "scenario"
    link_cmd = ["clang", *(str(obj) for obj in objects)]
    for value in target.get("link_inputs", []):
        link_cmd.append(str(resolve_input(project_root, value)))
    link_cmd += stage_cfg.get("link_args", []) + target.get("link_args", []) + ["-o", str(executable)]
    link_result = run(link_cmd, project_root, timeout, dry_run)
    save_result(artifact_root / lane, "link", link_cmd, link_result)
    return {
        "ok": link_result["ok"],
        "phase": "ready" if link_result["ok"] else "link",
        "commands": results,
        "link": {"command": link_cmd, **link_result},
        "executable": executable,
    }


def main() -> int:
    args = parse_args()
    if args.repeat < 2:
        raise SystemExit("Stage G requires --repeat >= 2")
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    project = next((p for p in manifest["projects"] if p.get("name") == args.project), None)
    if not project:
        raise SystemExit(f"unknown project: {args.project}")
    stage_cfg = project.get("stages", {}).get(STAGE)
    if not stage_cfg or not stage_cfg.get("enabled"):
        raise SystemExit(f"stage disabled or missing: {STAGE}")
    all_targets = list(stage_cfg.get("targets", []))
    available_count = len(all_targets)
    try:
        targets, selection_kind = select_targets(all_targets, args.target)
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc
    canonical = canonical_stage_closure_enabled(
        selection_kind=selection_kind, clang_parity_enabled=True, dry_run=args.dry_run)
    project_root = resolve_root(project["root"])
    preprocessor: list[str] = []
    for value in merged(project, "include_dirs"):
        path = Path(value)
        if not path.is_absolute():
            path = (project_root / path).resolve()
        preprocessor.append(f"-I{path}")
    preprocessor += [f"-D{value}" for value in merged(project, "defines")]
    history_id = time.strftime("%Y%m%dT%H%M%S") + f"_{time.time_ns() % 1000000000:09d}"
    latest_artifacts = latest_artifact_dir(
        ARTIFACT_ROOT, project_name=args.project, stage_key=STAGE, canonical=canonical)
    history_artifacts = history_artifact_dir(
        ARTIFACT_ROOT, history_id=history_id, project_name=args.project, stage_key=STAGE, canonical=canonical)
    latest_artifacts.mkdir(parents=True, exist_ok=True)
    report_targets: list[dict[str, Any]] = []
    blockers = 0
    with tempfile.TemporaryDirectory(prefix="fisics-stageg-") as temp:
        temp_root = Path(temp)
        for target in targets:
            target_artifacts = latest_artifacts / target["id"]
            timeout = args.timeout_sec or int(target.get("timeout_sec", stage_cfg.get("timeout_sec", 240)))
            clang_build = compile_lane("clang", "clang", project_root, target, preprocessor, stage_cfg, temp_root / target["id"], target_artifacts, timeout, args.dry_run)
            fisics_build = compile_lane("fisics", FISICS_ROOT / "fisics", project_root, target, preprocessor, stage_cfg, temp_root / target["id"], target_artifacts, timeout, args.dry_run)
            clang_report = {**clang_build, "executable": str(clang_build["executable"]) if clang_build["executable"] else None}
            fisics_report = {**fisics_build, "executable": str(fisics_build["executable"]) if fisics_build["executable"] else None}
            row: dict[str, Any] = {"target": target["id"], "clang_build": clang_report, "fisics_build": fisics_report, "runs": {"clang": [], "fisics": []}}
            expected = list(target.get("expected_checkpoints", []))
            if not clang_build["ok"] or not fisics_build["ok"] or not expected:
                row["status"] = "build_or_contract_fail"
                blockers += 1
                report_targets.append(row)
                continue
            lane_traces: dict[str, list[list[str]]] = {"clang": [], "fisics": []}
            lane_artifacts: dict[str, list[dict[str, str]]] = {"clang": [], "fisics": []}
            artifact_contract = list(target.get("expected_artifacts", []))
            expected_artifact_paths = sorted(str(item["path"]) for item in artifact_contract)
            compared_artifact_paths = {
                str(item["path"]) for item in artifact_contract if item.get("compare") == "sha256"
            }
            for lane, build in (("clang", clang_build), ("fisics", fisics_build)):
                for repeat_index in range(args.repeat):
                    run_dir = temp_root / target["id"] / lane / f"run_{repeat_index + 1}"
                    run_dir.mkdir(parents=True, exist_ok=True)
                    fixtures, fixture_paths = stage_runtime_fixtures(
                        project_root,
                        list(target.get("runtime_fixtures", [])),
                        run_dir,
                    )
                    run_env, applied_env = build_run_environment(
                        dict(os.environ),
                        list(target.get("scrub_env_prefixes", stage_cfg.get("scrub_env_prefixes", []))),
                        dict(stage_cfg.get("run_env", {})) | dict(target.get("run_env", {})),
                        project_root,
                        run_dir,
                    )
                    cmd = [str(build["executable"]), *target.get("run_args", [])]
                    result = run(
                        cmd,
                        run_dir,
                        int(target.get("run_timeout_sec", stage_cfg.get("run_timeout_sec", 30))),
                        args.dry_run,
                        env=run_env,
                    )
                    ok_trace, trace, detail = trace_contract(result["stdout"], expected)
                    actual_artifact_paths = sorted(
                        str(path.relative_to(run_dir))
                        for path in run_dir.rglob("*")
                        if path.is_file() and str(path.relative_to(run_dir)) not in fixture_paths
                    )
                    unexpected = sorted(set(actual_artifact_paths) - set(expected_artifact_paths))
                    missing = sorted(set(expected_artifact_paths) - set(actual_artifact_paths))
                    artifact_digests: dict[str, str] = {}
                    for relative_path in actual_artifact_paths:
                        source_path = run_dir / relative_path
                        artifact_digests[relative_path] = hashlib.sha256(source_path.read_bytes()).hexdigest()
                        destination = target_artifacts / lane / f"run_{repeat_index + 1}.artifacts" / relative_path
                        destination.parent.mkdir(parents=True, exist_ok=True)
                        shutil.copy2(source_path, destination)
                    run_ok = result["ok"] and result["returncode"] == int(target.get("expected_exit_code", 0)) and ok_trace and not unexpected and not missing
                    save_result(target_artifacts / lane, f"run_{repeat_index + 1}", cmd, result)
                    finalized_fixtures = finalize_fixture_provenance(run_dir, fixtures)
                    provenance_root = target_artifacts / lane
                    (provenance_root / f"run_{repeat_index + 1}.environment.json").write_text(
                        json.dumps({"scrub_prefixes": list(target.get("scrub_env_prefixes", stage_cfg.get("scrub_env_prefixes", []))), "applied": applied_env}, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8",
                    )
                    (provenance_root / f"run_{repeat_index + 1}.fixtures.json").write_text(
                        json.dumps(finalized_fixtures, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8",
                    )
                    row["runs"][lane].append({**result, "trace_ok": ok_trace, "trace_detail": detail, "unexpected_artifacts": unexpected, "missing_artifacts": missing, "artifact_sha256": artifact_digests, "trace_sha256": hashlib.sha256(result["stdout"].encode()).hexdigest(), "environment": {"scrub_prefixes": list(target.get("scrub_env_prefixes", stage_cfg.get("scrub_env_prefixes", []))), "applied": applied_env}, "fixtures": finalized_fixtures})
                    lane_traces[lane].append(trace)
                    lane_artifacts[lane].append({path: artifact_digests[path] for path in compared_artifact_paths if path in artifact_digests})
                    if not run_ok:
                        blockers += 1
            deterministic = all(traces and all(trace == traces[0] for trace in traces) for traces in lane_traces.values())
            parity = deterministic and lane_traces["clang"][0] == lane_traces["fisics"][0]
            artifact_deterministic = all(
                artifacts and all(item == artifacts[0] for item in artifacts)
                for artifacts in lane_artifacts.values()
            ) if compared_artifact_paths else True
            artifact_parity = artifact_deterministic and (
                lane_artifacts["clang"][0] == lane_artifacts["fisics"][0]
            ) if compared_artifact_paths else True
            row["deterministic"] = deterministic
            row["trace_parity"] = parity
            row["artifact_deterministic"] = artifact_deterministic
            row["artifact_parity"] = artifact_parity
            row["status"] = "both_pass" if deterministic and parity and artifact_deterministic and artifact_parity and all(r["ok"] and r["trace_ok"] and not r["unexpected_artifacts"] and not r["missing_artifacts"] for runs in row["runs"].values() for r in runs) else "differential_fail"
            if row["status"] != "both_pass":
                blockers += 1
            report_targets.append(row)
    report = {
        "schema_version": 1,
        "stage": STAGE,
        "trace_contract_version": 1,
        "project": {"name": args.project, "root": str(project_root), "git": git_meta(project_root)},
        "fisics": {"root": str(FISICS_ROOT), "git": git_meta(FISICS_ROOT)},
        "repeat": args.repeat,
        "targets": report_targets,
        "summary": {"targets": len(report_targets), "both_pass": sum(r.get("status") == "both_pass" for r in report_targets), "blockers": blockers},
        "report_contract": build_report_contract(
            report_family="real_project_operational_differential",
            selection_kind=selection_kind,
            canonical_stage_closure=canonical,
            selected_count=len(targets),
            available_count=available_count,
            selector={"target": args.target},
            lane_flags={"clang_parity": True, "repeat": args.repeat, "dry_run": args.dry_run},
        ),
    }
    latest_report, history_report = write_json_report(
        REPORT_ROOT, project_name=args.project, report_key=STAGE,
        history_id=history_id, canonical=canonical, payload=report)
    history_artifacts.parent.mkdir(parents=True, exist_ok=True)
    if latest_artifacts.exists() and not history_artifacts.exists():
        shutil.copytree(latest_artifacts, history_artifacts)
    print(f"project={args.project} stage={STAGE} targets={len(report_targets)} both_pass={report['summary']['both_pass']} blockers={blockers}")
    print(f"latest_report={latest_report}")
    print(f"history_report={history_report}")
    print(f"latest_artifacts={latest_artifacts}")
    return 0 if blockers == 0 else 2


if __name__ == "__main__":
    raise SystemExit(main())
