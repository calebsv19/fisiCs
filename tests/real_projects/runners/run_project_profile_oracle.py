#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import run_project_compile_tests as stage_a


SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
FISICS_ROOT = REAL_PROJECTS_ROOT.parent.parent
DEFAULT_ARTIFACT_ROOT = REAL_PROJECTS_ROOT / "artifacts"
DEFAULT_STAGE_KEY = "A_tu_compile"
DEFAULT_TIMEOUT_SEC = 120


@dataclass
class CommandResult:
    ok: bool
    returncode: int | None
    timed_out: bool
    duration_ms: int
    stdout: str
    stderr: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Profile one real-project translation unit using the exact fisiCs "
            "Stage-A compile command shape and validate that the profile CSV "
            "contains real samples."
        )
    )
    parser.add_argument("--manifest", type=Path, default=stage_a.DEFAULT_MANIFEST)
    parser.add_argument("--project", required=True, help="Project name in manifest.")
    parser.add_argument(
        "--source",
        default="",
        help="Exact project-relative source path to profile.",
    )
    parser.add_argument(
        "--filter",
        default="",
        help="Substring filter used to select one project-relative source path.",
    )
    parser.add_argument(
        "--timeout-sec",
        type=int,
        default=0,
        help="Override command timeout in seconds.",
    )
    parser.add_argument(
        "--top",
        type=int,
        default=20,
        help="Number of top timed buckets to print.",
    )
    parser.add_argument(
        "--keep-csv",
        action="store_true",
        help="Keep the temporary profile CSV in addition to the saved artifact copy.",
    )
    return parser.parse_args()


def pick_source(project_root: Path, project: dict[str, Any], args: argparse.Namespace) -> Path:
    globs = list(project.get("file_globs", []))
    excludes = list(project.get("exclude_globs", []))
    sources = stage_a.collect_sources(project_root, globs, excludes)
    if args.source:
        target = (project_root / args.source).resolve()
        if target not in sources:
            raise RuntimeError(f"source not found in project selection: {args.source}")
        return target

    if args.filter:
        matches = [path for path in sources if args.filter in path.relative_to(project_root).as_posix()]
        if not matches:
            raise RuntimeError(f"no sources matched filter={args.filter}")
        if len(matches) > 1:
            sample = "\n".join(f"  - {path.relative_to(project_root).as_posix()}" for path in matches[:10])
            raise RuntimeError(
                f"filter matched {len(matches)} sources; provide --source or narrow --filter:\n{sample}"
            )
        return matches[0]

    raise RuntimeError("provide either --source or --filter")


def build_fisics_cmd(project_root: Path, project: dict[str, Any], source: Path, object_path: Path) -> list[str]:
    stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
    fisics_extra_args = list(stage_cfg.get("fisics_extra_args", []))
    include_dirs = stage_a.merged_project_lists(project, "include_dirs")
    defines = stage_a.merged_project_lists(project, "defines")
    pp_flags = stage_a.render_preprocessor_flags(project_root, include_dirs, defines)
    fisics_bin = (FISICS_ROOT / "fisics").resolve()
    return [str(fisics_bin)] + pp_flags + fisics_extra_args + ["-c", str(source), "-o", str(object_path)]


def run_cmd(cmd: list[str], cwd: Path, timeout_sec: int, profile_csv: Path) -> CommandResult:
    env = dict(os.environ)
    env["FISICS_PROFILE"] = "1"
    env["FISICS_PROFILE_STREAM"] = "1"
    env["FISICS_PROFILE_PATH"] = str(profile_csv)

    t0 = time.perf_counter()
    try:
        completed = subprocess.run(
            cmd,
            cwd=str(cwd),
            env=env,
            text=True,
            capture_output=True,
            timeout=timeout_sec,
            check=False,
        )
        elapsed_ms = int((time.perf_counter() - t0) * 1000.0)
        return CommandResult(
            ok=(completed.returncode == 0),
            returncode=completed.returncode,
            timed_out=False,
            duration_ms=elapsed_ms,
            stdout=completed.stdout,
            stderr=completed.stderr,
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
        )


def parse_profile_csv(path: Path) -> dict[str, Any]:
    if not path.exists():
        raise RuntimeError(f"profile CSV was not created: {path}")

    timed_totals_ms: dict[str, float] = defaultdict(float)
    scalar_totals: dict[str, int] = defaultdict(int)
    row_count = 0
    with path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            row_count += 1
            name = (row.get("name") or "").strip()
            if not name:
                continue
            duration_ns = int((row.get("duration_ns") or "0").strip() or "0")
            duration_ms = float((row.get("duration_ms") or "0").strip() or "0")
            if duration_ms > 0.0:
                timed_totals_ms[name] += duration_ms
            elif duration_ns != 0:
                scalar_totals[name] += duration_ns

    if row_count == 0:
        raise RuntimeError(f"profile CSV only contains the header row: {path}")
    if not timed_totals_ms:
        raise RuntimeError(f"profile CSV contains no timed samples: {path}")

    top_timed = sorted(timed_totals_ms.items(), key=lambda item: (-item[1], item[0]))
    top_scalars = sorted(scalar_totals.items(), key=lambda item: (-item[1], item[0]))
    return {
        "row_count": row_count,
        "timed_totals_ms": top_timed,
        "scalar_totals": top_scalars,
    }


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")


def save_artifacts(
    project_name: str,
    rel_source: Path,
    cmd: list[str],
    result: CommandResult,
    profile_csv: Path,
    parsed_profile: dict[str, Any],
) -> tuple[Path, Path]:
    stage_dir = DEFAULT_ARTIFACT_ROOT / "latest" / project_name / "profile_oracle"
    stem = stage_dir / rel_source.with_suffix("")
    cmd_path = stem.with_suffix(".fisics.cmd.txt")
    stdout_path = stem.with_suffix(".fisics.stdout.txt")
    stderr_path = stem.with_suffix(".fisics.stderr.txt")
    csv_path = stem.with_suffix(".profile.csv")
    summary_path = stem.with_suffix(".profile.summary.json")

    write_text(cmd_path, " ".join(cmd) + "\n")
    write_text(stdout_path, result.stdout)
    write_text(stderr_path, result.stderr)
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(profile_csv, csv_path)

    summary = {
        "project": project_name,
        "source": rel_source.as_posix(),
        "command": cmd,
        "duration_ms": result.duration_ms,
        "returncode": result.returncode,
        "timed_out": result.timed_out,
        "profile_rows": parsed_profile["row_count"],
        "top_timed": [
            {"name": name, "total_ms": total_ms}
            for name, total_ms in parsed_profile["timed_totals_ms"][:50]
        ],
        "top_scalars": [
            {"name": name, "value": value}
            for name, value in parsed_profile["scalar_totals"][:50]
        ],
    }
    write_text(summary_path, json.dumps(summary, indent=2) + "\n")
    return csv_path, summary_path


def print_summary(
    project_name: str,
    rel_source: Path,
    result: CommandResult,
    parsed_profile: dict[str, Any],
    csv_path: Path,
    summary_path: Path,
    top_n: int,
) -> None:
    print(f"project={project_name} source={rel_source.as_posix()}")
    print(
        f"status={'ok' if result.ok else 'fail'} returncode={result.returncode} "
        f"timed_out={int(result.timed_out)} duration_ms={result.duration_ms}"
    )
    print(f"profile_rows={parsed_profile['row_count']}")
    print("top_timed:")
    for name, total_ms in parsed_profile["timed_totals_ms"][:top_n]:
        print(f"  {name} {total_ms:.3f}ms")
    if parsed_profile["scalar_totals"]:
        print("top_scalars:")
        for name, value in parsed_profile["scalar_totals"][: min(top_n, 10)]:
            print(f"  {name} {value}")
    print(f"saved_csv={csv_path}")
    print(f"saved_summary={summary_path}")


def main() -> int:
    args = parse_args()
    try:
        manifest = stage_a.load_manifest(args.manifest.resolve())
        project = stage_a.pick_project(manifest, args.project)
        project_root = stage_a.resolve_project_root(str(project["root"]))
        source = pick_source(project_root, project, args)
        rel_source = source.relative_to(project_root)
        stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
        timeout_sec = args.timeout_sec or int(stage_cfg.get("timeout_sec", DEFAULT_TIMEOUT_SEC))

        with tempfile.TemporaryDirectory(prefix="fisics-profile-oracle-") as tmp:
            tmp_root = Path(tmp)
            obj_path = tmp_root / rel_source.with_suffix(".o")
            obj_path.parent.mkdir(parents=True, exist_ok=True)
            profile_csv = tmp_root / rel_source.with_suffix(".csv")
            profile_csv.parent.mkdir(parents=True, exist_ok=True)

            cmd = build_fisics_cmd(project_root, project, source, obj_path)
            result = run_cmd(cmd=cmd, cwd=project_root, timeout_sec=timeout_sec, profile_csv=profile_csv)
            if not result.ok:
                sys.stderr.write(result.stderr)
                if result.stdout:
                    sys.stderr.write(result.stdout)
                raise RuntimeError(
                    f"profiled compile failed for {rel_source.as_posix()} "
                    f"(returncode={result.returncode}, timed_out={result.timed_out})"
                )

            parsed_profile = parse_profile_csv(profile_csv)
            csv_path, summary_path = save_artifacts(
                project_name=project["name"],
                rel_source=rel_source,
                cmd=cmd,
                result=result,
                profile_csv=profile_csv,
                parsed_profile=parsed_profile,
            )
            print_summary(
                project_name=project["name"],
                rel_source=rel_source,
                result=result,
                parsed_profile=parsed_profile,
                csv_path=csv_path,
                summary_path=summary_path,
                top_n=args.top,
            )
            if args.keep_csv:
                kept_path = Path.cwd() / f"{rel_source.stem}.profile.csv"
                shutil.copyfile(profile_csv, kept_path)
                print(f"kept_csv={kept_path}")
        return 0
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
