from __future__ import annotations

import json
import subprocess
from pathlib import Path
from typing import Any


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


def classify_selection_kind(
    *,
    selected_count: int,
    available_count: int,
    filter_value: str = "",
    target_value: str = "",
    limit: int = 0,
) -> str:
    if filter_value or target_value or limit > 0 or selected_count != available_count:
        return "filtered"
    return "full"


def canonical_stage_closure_enabled(
    *,
    selection_kind: str,
    clang_parity_enabled: bool,
    dry_run: bool,
) -> bool:
    return selection_kind == "full" and clang_parity_enabled and not dry_run


def latest_scope(canonical_stage_closure: bool) -> str:
    return "canonical" if canonical_stage_closure else "noncanonical"


def build_report_contract(
    *,
    report_family: str,
    selection_kind: str,
    canonical_stage_closure: bool,
    selected_count: int,
    available_count: int,
    selector: dict[str, Any] | None = None,
    lane_flags: dict[str, Any] | None = None,
) -> dict[str, Any]:
    return {
        "version": 1,
        "report_family": report_family,
        "selection_kind": selection_kind,
        "canonical_stage_closure": canonical_stage_closure,
        "latest_scope": latest_scope(canonical_stage_closure),
        "selected_count": selected_count,
        "available_count": available_count,
        "selector": selector or {},
        "lane_flags": lane_flags or {},
    }


def latest_report_path(
    report_root: Path,
    *,
    project_name: str,
    report_key: str,
    canonical: bool,
) -> Path:
    latest_dir = report_root / "latest"
    if not canonical:
        latest_dir = latest_dir / "noncanonical"
    return latest_dir / f"{project_name}_{report_key}_latest.json"


def history_report_path(
    report_root: Path,
    *,
    history_id: str,
    project_name: str,
    report_key: str,
    canonical: bool,
) -> Path:
    history_dir = report_root / "history"
    if not canonical:
        history_dir = history_dir / "noncanonical"
    return history_dir / f"{history_id}_{project_name}_{report_key}.json"


def canonical_latest_report_path(
    report_root: Path,
    *,
    project_name: str,
    report_key: str,
) -> Path:
    return latest_report_path(
        report_root,
        project_name=project_name,
        report_key=report_key,
        canonical=True,
    )


def latest_artifact_dir(
    artifact_root: Path,
    *,
    project_name: str,
    stage_key: str,
    canonical: bool,
) -> Path:
    latest_dir = artifact_root / "latest"
    if not canonical:
        latest_dir = latest_dir / "noncanonical"
    return latest_dir / project_name / stage_key


def history_artifact_dir(
    artifact_root: Path,
    *,
    history_id: str,
    project_name: str,
    stage_key: str,
    canonical: bool,
) -> Path:
    history_dir = artifact_root / "history"
    if not canonical:
        history_dir = history_dir / "noncanonical"
    return history_dir / history_id / project_name / stage_key


def write_json_report(
    report_root: Path,
    *,
    project_name: str,
    report_key: str,
    history_id: str,
    canonical: bool,
    payload: dict[str, Any],
) -> tuple[Path, Path]:
    latest_path = latest_report_path(
        report_root,
        project_name=project_name,
        report_key=report_key,
        canonical=canonical,
    )
    history_path = history_report_path(
        report_root,
        history_id=history_id,
        project_name=project_name,
        report_key=report_key,
        canonical=canonical,
    )
    latest_path.parent.mkdir(parents=True, exist_ok=True)
    history_path.parent.mkdir(parents=True, exist_ok=True)
    latest_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    history_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return latest_path, history_path
