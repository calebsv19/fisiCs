#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import platform
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
FISICS_ROOT = REAL_PROJECTS_ROOT.parent.parent
WORKSPACE_ROOT = FISICS_ROOT.parent
DEFAULT_MANIFEST = REAL_PROJECTS_ROOT / "config" / "projects_manifest.json"
DEFAULT_REPORT_ROOT = REAL_PROJECTS_ROOT / "reports"
DEFAULT_ARTIFACT_ROOT = REAL_PROJECTS_ROOT / "artifacts"
DEFAULT_STAGE_KEY = "A_tu_compile"


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
        description="Run real-project translation-unit compile validation (Stage A)."
    )
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--project", default=None, help="Project name in manifest.")
    parser.add_argument("--stage", default=DEFAULT_STAGE_KEY, choices=[DEFAULT_STAGE_KEY])
    parser.add_argument("--limit", type=int, default=0, help="Limit number of source files.")
    parser.add_argument("--filter", default="", help="Substring filter on source relative path.")
    parser.add_argument("--timeout-sec", type=int, default=0, help="Override stage timeout.")
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


def collect_sources(project_root: Path, globs: list[str], excludes: list[str]) -> list[Path]:
    files: set[Path] = set()
    for pattern in globs:
        for path in project_root.glob(pattern):
            if path.is_file():
                files.add(path.resolve())
    if excludes:
        excluded: set[Path] = set()
        for pattern in excludes:
            for path in project_root.glob(pattern):
                if path.is_file():
                    excluded.add(path.resolve())
        files -= excluded
    return sorted(files, key=lambda p: p.as_posix())


def render_preprocessor_flags(project_root: Path, include_dirs: list[str], defines: list[str]) -> list[str]:
    flags: list[str] = []
    for include_dir in include_dirs:
        include_path = Path(include_dir)
        if not include_path.is_absolute():
            include_path = (project_root / include_path).resolve()
        flags.append(f"-I{include_path}")
    for define in defines:
        flags.append(f"-D{define}")
    return flags


def run_cmd(cmd: list[str], cwd: Path, timeout_sec: int, dry_run: bool) -> CommandResult:
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
        "control reaches end of non-void function",
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


def parity_label(fisics: CommandResult, clang: CommandResult | None) -> str:
    if clang is None:
        return "fisics_only"
    if fisics.ok and clang.ok:
        return "both_pass"
    if (not fisics.ok) and clang.ok:
        return "fisics_only_fail"
    if fisics.ok and (not clang.ok):
        return "clang_only_fail"
    if fisics.timed_out and clang.timed_out:
        return "both_timeout"
    return "both_fail"


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        f.write(value)


def save_artifacts(
    stage_dir: Path,
    rel_source: Path,
    fisics_cmd: list[str],
    fisics_result: CommandResult,
    clang_cmd: list[str] | None,
    clang_result: CommandResult | None,
) -> None:
    stem = rel_source.with_suffix("")
    command_prefix = stage_dir / stem
    write_text(command_prefix.with_suffix(".fisics.cmd.txt"), " ".join(fisics_cmd) + "\n")
    write_text(command_prefix.with_suffix(".fisics.stdout.txt"), fisics_result.stdout)
    write_text(command_prefix.with_suffix(".fisics.stderr.txt"), fisics_result.stderr)
    if clang_cmd is not None and clang_result is not None:
        write_text(command_prefix.with_suffix(".clang.cmd.txt"), " ".join(clang_cmd) + "\n")
        write_text(command_prefix.with_suffix(".clang.stdout.txt"), clang_result.stdout)
        write_text(command_prefix.with_suffix(".clang.stderr.txt"), clang_result.stderr)


def run_stage_a(
    project: dict[str, Any],
    project_root: Path,
    args: argparse.Namespace,
) -> dict[str, Any]:
    stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
    timeout_sec = args.timeout_sec or int(stage_cfg.get("timeout_sec", 45))
    fisics_extra_args = list(stage_cfg.get("fisics_extra_args", []))
    clang_extra_args = list(stage_cfg.get("clang_extra_args", []))

    include_dirs = merged_project_lists(project, "include_dirs")
    defines = merged_project_lists(project, "defines")
    pp_flags = render_preprocessor_flags(project_root, include_dirs, defines)
    globs = list(project.get("file_globs", []))
    excludes = list(project.get("exclude_globs", []))
    sources = collect_sources(project_root, globs, excludes)
    if args.filter:
        sources = [s for s in sources if args.filter in s.relative_to(project_root).as_posix()]
    if args.limit > 0:
        sources = sources[: args.limit]

    latest_stage_dir = (
        DEFAULT_ARTIFACT_ROOT / "latest" / project["name"] / DEFAULT_STAGE_KEY
    )
    history_id = f"{time.strftime('%Y%m%dT%H%M%S')}_{time.time_ns() % 1_000_000_000:09d}"
    history_stage_dir = (
        DEFAULT_ARTIFACT_ROOT / "history" / history_id / project["name"] / DEFAULT_STAGE_KEY
    )
    latest_stage_dir.mkdir(parents=True, exist_ok=True)

    fisics_bin = (FISICS_ROOT / "fisics").resolve()
    if not fisics_bin.exists():
        raise RuntimeError(f"fisics binary not found: {fisics_bin}")

    results: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="fisics-realproj-") as tmp:
        tmp_root = Path(tmp)
        for source in sources:
            rel_source = source.relative_to(project_root)
            fisics_obj = tmp_root / "fisics" / rel_source.with_suffix(".o")
            fisics_obj.parent.mkdir(parents=True, exist_ok=True)
            fisics_cmd = (
                [str(fisics_bin)]
                + pp_flags
                + fisics_extra_args
                + ["-c", str(source), "-o", str(fisics_obj)]
            )
            fisics_result = run_cmd(
                cmd=fisics_cmd,
                cwd=project_root,
                timeout_sec=timeout_sec,
                dry_run=args.dry_run,
            )

            clang_cmd: list[str] | None = None
            clang_result: CommandResult | None = None
            if not args.skip_clang:
                clang_obj = tmp_root / "clang" / rel_source.with_suffix(".o")
                clang_obj.parent.mkdir(parents=True, exist_ok=True)
                clang_cmd = (
                    ["clang"]
                    + clang_extra_args
                    + pp_flags
                    + ["-c", str(source), "-o", str(clang_obj)]
                )
                clang_result = run_cmd(
                    cmd=clang_cmd,
                    cwd=project_root,
                    timeout_sec=timeout_sec,
                    dry_run=args.dry_run,
                )

            save_artifacts(
                stage_dir=latest_stage_dir,
                rel_source=rel_source,
                fisics_cmd=fisics_cmd,
                fisics_result=fisics_result,
                clang_cmd=clang_cmd,
                clang_result=clang_result,
            )

            fisics_failure_class = None if fisics_result.ok else classify_failure(fisics_result)
            clang_failure_class = None
            if clang_result is not None and not clang_result.ok:
                clang_failure_class = classify_failure(clang_result)
            parity = parity_label(fisics_result, clang_result)
            is_blocker = (not fisics_result.ok) and (clang_result is None or clang_result.ok)

            results.append(
                {
                    "source": rel_source.as_posix(),
                    "parity": parity,
                    "is_blocker": is_blocker,
                    "fisics": {
                        "ok": fisics_result.ok,
                        "returncode": fisics_result.returncode,
                        "timed_out": fisics_result.timed_out,
                        "duration_ms": fisics_result.duration_ms,
                        "signal": fisics_result.signal,
                        "failure_class": fisics_failure_class,
                    },
                    "clang": None
                    if clang_result is None
                    else {
                        "ok": clang_result.ok,
                        "returncode": clang_result.returncode,
                        "timed_out": clang_result.timed_out,
                        "duration_ms": clang_result.duration_ms,
                        "signal": clang_result.signal,
                        "failure_class": clang_failure_class,
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
    for row in results:
        parity_counts[row["parity"]] = parity_counts.get(row["parity"], 0) + 1
        if row["is_blocker"]:
            blocker_count += 1

    fisics_total_ms = sum(row["fisics"]["duration_ms"] for row in results)
    clang_total_ms = 0
    if not args.skip_clang:
        clang_total_ms = sum(row["clang"]["duration_ms"] for row in results if row["clang"] is not None)

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
        "sources_total": len(results),
        "timeout_sec": timeout_sec,
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
    print(f"project={report['project']['name']} stage={report['stage']} sources={report['sources_total']}")
    print(f"blockers={summary['blockers']} parity={json.dumps(parity_counts, sort_keys=True)}")
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
        report = run_stage_a(project=project, project_root=project_root, args=args)
        latest_path, history_path = write_report(report)
        print_summary(report, latest_path=latest_path, history_path=history_path)
        return 2 if report["summary"]["blockers"] > 0 else 0
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
