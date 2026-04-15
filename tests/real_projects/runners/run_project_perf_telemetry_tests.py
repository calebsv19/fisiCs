#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
FISICS_ROOT = REAL_PROJECTS_ROOT.parent.parent
WORKSPACE_ROOT = FISICS_ROOT.parent
DEFAULT_MANIFEST = REAL_PROJECTS_ROOT / "config" / "projects_manifest.json"
DEFAULT_REPORT_ROOT = REAL_PROJECTS_ROOT / "reports"
DEFAULT_ARTIFACT_ROOT = REAL_PROJECTS_ROOT / "artifacts"
DEFAULT_STAGE_KEY = "F_perf_telemetry"
DEFAULT_REQUIRED_STAGES = [
    "A_tu_compile",
    "B_link_subset",
    "C_full_build",
    "D_runtime_smoke",
    "E_golden_behavior",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run real-project performance telemetry snapshot (Stage F)."
    )
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--project", default=None, help="Project name in manifest.")
    parser.add_argument("--stage", default=DEFAULT_STAGE_KEY, choices=[DEFAULT_STAGE_KEY])
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Collect metadata only without writing reports/artifacts.",
    )
    return parser.parse_args()


def run_checked(cmd: list[str], cwd: Path) -> tuple[int, str]:
    completed = subprocess.run(
        cmd,
        cwd=str(cwd),
        text=True,
        capture_output=True,
        check=False,
    )
    out = (completed.stdout or "") + (completed.stderr or "")
    return completed.returncode, out.strip()


def git_meta(repo_root: Path) -> dict[str, Any]:
    commit = "unknown"
    branch = "unknown"
    dirty = None

    rc, out = run_checked(["git", "-C", str(repo_root), "rev-parse", "HEAD"], cwd=repo_root)
    if rc == 0 and out:
        commit = out.splitlines()[-1].strip()

    rc, out = run_checked(
        ["git", "-C", str(repo_root), "rev-parse", "--abbrev-ref", "HEAD"], cwd=repo_root
    )
    if rc == 0 and out:
        branch = out.splitlines()[-1].strip()

    rc, out = run_checked(["git", "-C", str(repo_root), "status", "--porcelain"], cwd=repo_root)
    if rc == 0:
        dirty = len(out.strip()) > 0

    return {
        "commit": commit,
        "branch": branch,
        "dirty": dirty,
    }


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


def read_latest_stage_report(project_name: str, stage_name: str) -> tuple[dict[str, Any] | None, Path]:
    path = DEFAULT_REPORT_ROOT / "latest" / f"{project_name}_{stage_name}_latest.json"
    if not path.exists():
        return None, path
    with path.open("r", encoding="utf-8") as f:
        return json.load(f), path


def choose_binary(project_root: Path, stage_cfg: dict[str, Any], project: dict[str, Any]) -> dict[str, Any]:
    candidates = list(stage_cfg.get("binary_candidates", []))
    if not candidates:
        default_candidate = project.get("binary_candidate")
        if isinstance(default_candidate, str) and default_candidate:
            candidates.append(default_candidate)

    for raw in candidates:
        p = Path(raw)
        if not p.is_absolute():
            p = (project_root / p).resolve()
        if p.exists() and p.is_file():
            return {
                "path": str(p),
                "size_bytes": p.stat().st_size,
            }
    return {
        "path": None,
        "size_bytes": None,
    }


def collect_stage_metrics(project_name: str, required_stages: list[str]) -> tuple[list[dict[str, Any]], list[str]]:
    stage_rows: list[dict[str, Any]] = []
    missing: list[str] = []

    for stage_name in required_stages:
        report, report_path = read_latest_stage_report(project_name, stage_name)
        if report is None:
            missing.append(stage_name)
            continue

        summary = report.get("summary", {})
        row = {
            "stage": stage_name,
            "report_path": str(report_path),
            "items_total": int(
                report.get("sources_total", report.get("targets_total", 0))
            ),
            "fisics_total_ms": int(summary.get("fisics_total_ms", 0)),
            "clang_total_ms": int(summary.get("clang_total_ms", 0)),
            "blockers": int(summary.get("blockers", 0)),
            "parity_counts": summary.get("parity_counts", {}),
        }
        stage_rows.append(row)

    return stage_rows, missing


def load_previous_stage_f(project_name: str, current_history_id: str) -> dict[str, Any] | None:
    history_dir = DEFAULT_REPORT_ROOT / "history"
    if not history_dir.exists():
        return None

    pattern = f"*_{project_name}_{DEFAULT_STAGE_KEY}.json"
    candidates = sorted(history_dir.glob(pattern))
    if not candidates:
        return None

    current_name = f"{current_history_id}_{project_name}_{DEFAULT_STAGE_KEY}.json"
    for path in reversed(candidates):
        if path.name == current_name:
            continue
        try:
            with path.open("r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            continue
    return None


def stage_map(stage_rows: list[dict[str, Any]]) -> dict[str, dict[str, Any]]:
    return {row["stage"]: row for row in stage_rows}


def compute_warnings(
    stage_rows: list[dict[str, Any]],
    previous_report: dict[str, Any] | None,
    warn_pct: float,
    warn_min_ms: int,
    ratio_warn: float,
) -> list[str]:
    warnings: list[str] = []
    prev_rows = stage_map(previous_report.get("telemetry", {}).get("stages", [])) if previous_report else {}

    for row in stage_rows:
        stage = row["stage"]
        fisics_ms = int(row["fisics_total_ms"])
        clang_ms = int(row["clang_total_ms"])

        if clang_ms > 0:
            ratio = fisics_ms / clang_ms
            if ratio > ratio_warn:
                warnings.append(
                    f"{stage}: fisics/clang ratio {ratio:.2f} exceeds warn threshold {ratio_warn:.2f}"
                )

        prev = prev_rows.get(stage)
        if prev is None:
            continue

        prev_fisics = int(prev.get("fisics_total_ms", 0))
        if prev_fisics <= 0:
            continue

        increase_ms = fisics_ms - prev_fisics
        pct = (increase_ms / prev_fisics) * 100.0
        if increase_ms >= warn_min_ms and pct >= warn_pct:
            warnings.append(
                f"{stage}: fisics total increased {increase_ms}ms ({pct:.1f}%) vs previous Stage F snapshot"
            )

    return warnings


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        f.write(value)


def run_stage_f(project: dict[str, Any], project_root: Path, args: argparse.Namespace) -> dict[str, Any]:
    stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
    if not stage_cfg.get("enabled", False):
        raise RuntimeError(f"stage not enabled in manifest: {DEFAULT_STAGE_KEY}")

    required_stages = list(stage_cfg.get("required_stages", DEFAULT_REQUIRED_STAGES))
    warn_pct = float(stage_cfg.get("regression_warn_pct", 30.0))
    warn_min_ms = int(stage_cfg.get("regression_warn_min_ms", 250))
    ratio_warn = float(stage_cfg.get("ratio_warn", 20.0))
    hard_fail = bool(stage_cfg.get("hard_fail_on_regression", False))

    history_id = f"{time.strftime('%Y%m%dT%H%M%S')}_{time.time_ns() % 1_000_000_000:09d}"

    stage_rows, missing_stages = collect_stage_metrics(project_name=project["name"], required_stages=required_stages)
    fisics_total_ms = sum(int(row["fisics_total_ms"]) for row in stage_rows)
    clang_total_ms = sum(int(row["clang_total_ms"]) for row in stage_rows)

    previous_report = load_previous_stage_f(project_name=project["name"], current_history_id=history_id)
    warnings = compute_warnings(
        stage_rows=stage_rows,
        previous_report=previous_report,
        warn_pct=warn_pct,
        warn_min_ms=warn_min_ms,
        ratio_warn=ratio_warn,
    )

    if missing_stages:
        warnings.append(f"missing required stage reports: {', '.join(missing_stages)}")

    blockers = 0
    if missing_stages:
        blockers += len(missing_stages)
    if hard_fail:
        blockers += len(warnings)

    binary = choose_binary(project_root=project_root, stage_cfg=stage_cfg, project=project)
    fisics_git = git_meta(FISICS_ROOT)
    project_git = git_meta(project_root)

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
        "telemetry": {
            "required_stages": required_stages,
            "stages": stage_rows,
            "missing_stages": missing_stages,
            "totals": {
                "fisics_total_ms": fisics_total_ms,
                "clang_total_ms": clang_total_ms,
            },
            "git": {
                "fisics": fisics_git,
                "project": project_git,
            },
            "binary": binary,
            "thresholds": {
                "regression_warn_pct": warn_pct,
                "regression_warn_min_ms": warn_min_ms,
                "ratio_warn": ratio_warn,
                "hard_fail_on_regression": hard_fail,
            },
            "warnings": warnings,
        },
        "summary": {
            "parity_counts": {"telemetry": 1},
            "blockers": blockers,
            "warnings": len(warnings),
            "fisics_total_ms": fisics_total_ms,
            "clang_total_ms": clang_total_ms,
        },
    }

    if args.dry_run:
        return report

    latest_stage_dir = DEFAULT_ARTIFACT_ROOT / "latest" / project["name"] / DEFAULT_STAGE_KEY
    history_stage_dir = (
        DEFAULT_ARTIFACT_ROOT / "history" / history_id / project["name"] / DEFAULT_STAGE_KEY
    )
    latest_stage_dir.mkdir(parents=True, exist_ok=True)

    summary_txt_lines = [
        f"project={project['name']} stage={DEFAULT_STAGE_KEY}",
        f"required_stages={','.join(required_stages)}",
        f"missing_stages={','.join(missing_stages) if missing_stages else 'none'}",
        f"warnings={len(warnings)}",
        f"blockers={blockers}",
        f"timing_ms fisics={fisics_total_ms} clang={clang_total_ms}",
        f"fisics_git={fisics_git['commit']} dirty={fisics_git['dirty']}",
        f"project_git={project_git['commit']} dirty={project_git['dirty']}",
        f"binary_path={binary['path']}",
        f"binary_size_bytes={binary['size_bytes']}",
    ]
    if warnings:
        summary_txt_lines.append("warnings_detail:")
        summary_txt_lines.extend([f"- {w}" for w in warnings])

    write_text(latest_stage_dir / "summary.txt", "\n".join(summary_txt_lines) + "\n")
    write_text(latest_stage_dir / "summary.json", json.dumps(report, indent=2) + "\n")

    history_stage_dir.parent.mkdir(parents=True, exist_ok=True)
    if history_stage_dir.exists():
        shutil.rmtree(history_stage_dir)
    shutil.copytree(latest_stage_dir, history_stage_dir)

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
    print(
        f"project={report['project']['name']} stage={report['stage']} "
        f"stages_observed={len(report['telemetry']['stages'])}"
    )
    print(f"blockers={summary['blockers']} warnings={summary['warnings']}")
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
        report = run_stage_f(project=project, project_root=project_root, args=args)
        latest_path, history_path = write_report(report)
        print_summary(report, latest_path=latest_path, history_path=history_path)
        return 2 if report["summary"]["blockers"] > 0 else 0
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
