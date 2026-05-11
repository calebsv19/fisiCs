#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import os
import shutil
import statistics
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
DEFAULT_REPEAT = 3


@dataclass
class CommandResult:
    ok: bool
    returncode: int | None
    timed_out: bool
    duration_ms: int
    stdout: str
    stderr: str


@dataclass
class ParsedProfile:
    row_count: int
    timed_totals_ms: dict[str, float]
    scalar_totals: dict[str, int]


@dataclass
class OracleRun:
    run_index: int
    result: CommandResult
    parsed_profile: ParsedProfile


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Profile one real-project translation unit using the exact fisiCs "
            "Stage-A compile command shape, repeat it for a more stable median, "
            "and optionally compare the result against an earlier saved summary."
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
        help="Number of top timed/scalar buckets to print.",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=DEFAULT_REPEAT,
        help="Number of profiled runs to execute before reporting aggregate medians.",
    )
    parser.add_argument(
        "--warmup-runs",
        type=int,
        default=0,
        help="Number of unreported warmup runs to execute before measured runs.",
    )
    parser.add_argument(
        "--compare-summary",
        type=Path,
        default=None,
        help="Optional saved summary JSON to compare against. If omitted, compare against the existing saved summary for this TU when present.",
    )
    parser.add_argument(
        "--keep-csv",
        action="store_true",
        help="Keep the representative run CSV in the current directory in addition to artifact copies.",
    )
    parser.add_argument(
        "--extra-arg",
        action="append",
        default=[],
        help=(
            "Extra compiler argument to append to the saved Stage-A fisiCs command "
            "shape. Repeat this flag to pass multiple arguments."
        ),
    )
    parser.add_argument(
        "--artifact-label",
        default="",
        help=(
            "Optional suffix used to keep separate artifact sets for alternate "
            "command shapes on the same source."
        ),
    )
    parser.add_argument(
        "--set-env",
        action="append",
        default=[],
        help="Environment override in NAME=VALUE form. Repeat to set multiple values.",
    )
    parser.add_argument(
        "--profile-mode",
        choices=("timing", "both"),
        default="timing",
        help=(
            "Profiler mode for this attribution run. "
            "'timing' keeps counters off; 'both' enables timing plus scalar counters."
        ),
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


def build_fisics_cmd(
    project_root: Path,
    project: dict[str, Any],
    source: Path,
    object_path: Path,
    extra_args: list[str],
) -> list[str]:
    stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
    fisics_extra_args = list(stage_cfg.get("fisics_extra_args", []))
    include_dirs = stage_a.merged_project_lists(project, "include_dirs")
    defines = stage_a.merged_project_lists(project, "defines")
    pp_flags = stage_a.render_preprocessor_flags(project_root, include_dirs, defines)
    fisics_bin = (FISICS_ROOT / "fisics").resolve()
    return [str(fisics_bin)] + pp_flags + fisics_extra_args + list(extra_args) + ["-c", str(source), "-o", str(object_path)]


def parse_env_overrides(items: list[str]) -> dict[str, str]:
    env: dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise RuntimeError(f"invalid --set-env value (expected NAME=VALUE): {item}")
        name, value = item.split("=", 1)
        name = name.strip()
        if not name:
            raise RuntimeError(f"invalid --set-env value (missing name): {item}")
        env[name] = value
    return env


def run_cmd(
    cmd: list[str],
    cwd: Path,
    timeout_sec: int,
    profile_csv: Path,
    profile_mode: str,
    env_overrides: dict[str, str],
) -> CommandResult:
    env = dict(os.environ)
    env.pop("FISICS_PROFILE_MODE", None)
    env["FISICS_PROFILE"] = "1"
    env["FISICS_PROFILE_MODE"] = profile_mode
    env["FISICS_PROFILE_STREAM"] = "1"
    env["FISICS_PROFILE_PATH"] = str(profile_csv)
    env.update(env_overrides)

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


def parse_profile_csv(path: Path) -> ParsedProfile:
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

    return ParsedProfile(
        row_count=row_count,
        timed_totals_ms=dict(timed_totals_ms),
        scalar_totals=dict(scalar_totals),
    )


def sorted_timed_items(values: dict[str, float]) -> list[tuple[str, float]]:
    return sorted(values.items(), key=lambda item: (-item[1], item[0]))


def sorted_scalar_items(values: dict[str, int]) -> list[tuple[str, int]]:
    return sorted(values.items(), key=lambda item: (-item[1], item[0]))


def compute_median_map(
    per_run_maps: list[dict[str, float | int]],
    *,
    zero_value: float | int,
) -> dict[str, float]:
    names: set[str] = set()
    for values in per_run_maps:
        names.update(values.keys())
    medians: dict[str, float] = {}
    for name in names:
        samples = [float(values.get(name, zero_value)) for values in per_run_maps]
        median_value = float(statistics.median(samples))
        if median_value != 0.0:
            medians[name] = median_value
    return medians


def build_aggregate_summary(
    project_name: str,
    rel_source: Path,
    cmd: list[str],
    profile_mode: str,
    runs: list[OracleRun],
) -> dict[str, Any]:
    if not runs:
        raise RuntimeError("cannot build aggregate summary with zero runs")

    durations = [run.result.duration_ms for run in runs]
    row_counts = [run.parsed_profile.row_count for run in runs]
    timed_maps = [run.parsed_profile.timed_totals_ms for run in runs]
    scalar_maps = [run.parsed_profile.scalar_totals for run in runs]
    median_duration_ms = int(round(statistics.median(durations)))
    median_rows = int(round(statistics.median(row_counts)))
    median_timed = compute_median_map(timed_maps, zero_value=0.0)
    median_scalars = compute_median_map(scalar_maps, zero_value=0)
    representative_run = min(
        runs,
        key=lambda run: (abs(run.result.duration_ms - median_duration_ms), run.run_index),
    )

    return {
        "summary_version": 2,
        "project": project_name,
        "source": rel_source.as_posix(),
        "command": cmd,
        "profile_mode": profile_mode,
        "repeat": len(runs),
        "durations_ms": durations,
        "median_duration_ms": median_duration_ms,
        "min_duration_ms": min(durations),
        "max_duration_ms": max(durations),
        "mean_duration_ms": round(sum(durations) / len(durations), 3),
        "profile_rows_per_run": row_counts,
        "median_profile_rows": median_rows,
        "representative_run_index": representative_run.run_index,
        "runs": [
            {
                "run_index": run.run_index,
                "duration_ms": run.result.duration_ms,
                "returncode": run.result.returncode,
                "timed_out": run.result.timed_out,
                "profile_rows": run.parsed_profile.row_count,
                "top_timed": [
                    {"name": name, "total_ms": total_ms}
                    for name, total_ms in sorted_timed_items(run.parsed_profile.timed_totals_ms)[:50]
                ],
                "top_scalars": [
                    {"name": name, "value": value}
                    for name, value in sorted_scalar_items(run.parsed_profile.scalar_totals)[:50]
                ],
            }
            for run in runs
        ],
        "median_top_timed": [
            {"name": name, "total_ms": total_ms}
            for name, total_ms in sorted_timed_items(median_timed)[:50]
        ],
        "median_top_scalars": [
            {"name": name, "value": int(round(value))}
            for name, value in sorted_scalar_items({k: int(round(v)) for k, v in median_scalars.items()})[:50]
        ],
    }


def write_text(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")


def normalized_artifact_label(label: str) -> str:
    cleaned = "".join(ch if ch.isalnum() or ch in ("-", "_", ".") else "_" for ch in label.strip())
    return cleaned.strip("._")


def artifact_stem(project_name: str, rel_source: Path, artifact_label: str) -> Path:
    stage_dir = DEFAULT_ARTIFACT_ROOT / "latest" / project_name / "profile_oracle"
    stem = stage_dir / rel_source.with_suffix("")
    if not artifact_label:
        return stem
    return Path(f"{stem}.{normalized_artifact_label(artifact_label)}")


def read_summary_if_exists(path: Path | None) -> dict[str, Any] | None:
    if not path or not path.exists():
        return None
    return json.loads(path.read_text(encoding="utf-8"))


def summary_duration_ms(summary: dict[str, Any]) -> float:
    if "median_duration_ms" in summary:
        return float(summary["median_duration_ms"])
    return float(summary.get("duration_ms", 0))


def summary_timed_map(summary: dict[str, Any]) -> dict[str, float]:
    entries = summary.get("median_top_timed")
    if entries is None:
        entries = summary.get("top_timed", [])
    return {str(item["name"]): float(item["total_ms"]) for item in entries}


def summary_scalar_map(summary: dict[str, Any]) -> dict[str, int]:
    entries = summary.get("median_top_scalars")
    if entries is None:
        entries = summary.get("top_scalars", [])
    return {str(item["name"]): int(item["value"]) for item in entries}


def build_comparison(
    current_summary: dict[str, Any],
    baseline_summary: dict[str, Any],
) -> dict[str, Any]:
    current_duration = summary_duration_ms(current_summary)
    baseline_duration = summary_duration_ms(baseline_summary)
    current_timed = summary_timed_map(current_summary)
    baseline_timed = summary_timed_map(baseline_summary)
    current_scalars = summary_scalar_map(current_summary)
    baseline_scalars = summary_scalar_map(baseline_summary)

    timed_deltas: list[dict[str, Any]] = []
    for name in set(current_timed) | set(baseline_timed):
        current_value = current_timed.get(name, 0.0)
        baseline_value = baseline_timed.get(name, 0.0)
        delta = current_value - baseline_value
        if delta != 0.0:
            timed_deltas.append(
                {
                    "name": name,
                    "current_ms": current_value,
                    "baseline_ms": baseline_value,
                    "delta_ms": delta,
                    "abs_delta_ms": abs(delta),
                }
            )
    timed_deltas.sort(key=lambda item: (-item["abs_delta_ms"], item["name"]))

    scalar_deltas: list[dict[str, Any]] = []
    for name in set(current_scalars) | set(baseline_scalars):
        current_value = current_scalars.get(name, 0)
        baseline_value = baseline_scalars.get(name, 0)
        delta = current_value - baseline_value
        if delta != 0:
            scalar_deltas.append(
                {
                    "name": name,
                    "current_value": current_value,
                    "baseline_value": baseline_value,
                    "delta_value": delta,
                    "abs_delta_value": abs(delta),
                }
            )
    scalar_deltas.sort(key=lambda item: (-item["abs_delta_value"], item["name"]))

    return {
        "baseline_project": baseline_summary.get("project"),
        "baseline_source": baseline_summary.get("source"),
        "baseline_duration_ms": baseline_duration,
        "current_duration_ms": current_duration,
        "delta_duration_ms": current_duration - baseline_duration,
        "timed_deltas": timed_deltas,
        "scalar_deltas": scalar_deltas,
    }


def save_artifacts(
    project_name: str,
    rel_source: Path,
    cmd: list[str],
    profile_mode: str,
    runs: list[OracleRun],
    temp_csv_paths: list[Path],
    artifact_label: str,
) -> tuple[Path, Path, dict[str, Any]]:
    if len(runs) != len(temp_csv_paths):
        raise RuntimeError("run/csv path mismatch while saving oracle artifacts")

    stem = artifact_stem(project_name, rel_source, artifact_label)
    cmd_path = Path(f"{stem}.fisics.cmd.txt")
    aggregate_csv_path = Path(f"{stem}.profile.csv")
    aggregate_stdout_path = Path(f"{stem}.fisics.stdout.txt")
    aggregate_stderr_path = Path(f"{stem}.fisics.stderr.txt")
    summary_path = Path(f"{stem}.profile.summary.json")
    existing_summary = read_summary_if_exists(summary_path)

    write_text(cmd_path, " ".join(cmd) + "\n")

    aggregate_summary = build_aggregate_summary(project_name, rel_source, cmd, profile_mode, runs)
    representative_index = int(aggregate_summary["representative_run_index"]) - 1
    representative_run = runs[representative_index]

    for run, temp_csv_path in zip(runs, temp_csv_paths):
        run_label = f".run{run.run_index:02d}"
        run_csv_path = Path(f"{stem}{run_label}.profile.csv")
        run_stdout_path = Path(f"{stem}{run_label}.fisics.stdout.txt")
        run_stderr_path = Path(f"{stem}{run_label}.fisics.stderr.txt")
        run_summary_path = Path(f"{stem}{run_label}.profile.summary.json")
        run_csv_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(temp_csv_path, run_csv_path)
        write_text(run_stdout_path, run.result.stdout)
        write_text(run_stderr_path, run.result.stderr)
        run_summary = {
            "summary_version": 2,
            "project": project_name,
            "source": rel_source.as_posix(),
            "command": cmd,
            "profile_mode": profile_mode,
            "run_index": run.run_index,
            "duration_ms": run.result.duration_ms,
            "returncode": run.result.returncode,
            "timed_out": run.result.timed_out,
            "profile_rows": run.parsed_profile.row_count,
            "top_timed": [
                {"name": name, "total_ms": total_ms}
                for name, total_ms in sorted_timed_items(run.parsed_profile.timed_totals_ms)[:50]
            ],
            "top_scalars": [
                {"name": name, "value": value}
                for name, value in sorted_scalar_items(run.parsed_profile.scalar_totals)[:50]
            ],
        }
        write_text(run_summary_path, json.dumps(run_summary, indent=2) + "\n")

    shutil.copyfile(temp_csv_paths[representative_index], aggregate_csv_path)
    write_text(aggregate_stdout_path, representative_run.result.stdout)
    write_text(aggregate_stderr_path, representative_run.result.stderr)
    write_text(summary_path, json.dumps(aggregate_summary, indent=2) + "\n")
    return aggregate_csv_path, summary_path, existing_summary or {}


def print_summary(
    project_name: str,
    rel_source: Path,
    aggregate_summary: dict[str, Any],
    csv_path: Path,
    summary_path: Path,
    top_n: int,
    comparison: dict[str, Any] | None,
) -> None:
    durations = list(aggregate_summary["durations_ms"])
    print(f"project={project_name} source={rel_source.as_posix()}")
    print(
        f"repeat={aggregate_summary['repeat']} durations_ms={durations} "
        f"median_duration_ms={aggregate_summary['median_duration_ms']} "
        f"min_duration_ms={aggregate_summary['min_duration_ms']} "
        f"max_duration_ms={aggregate_summary['max_duration_ms']}"
    )
    print(
        f"median_profile_rows={aggregate_summary['median_profile_rows']} "
        f"representative_run={aggregate_summary['representative_run_index']}"
    )
    print("median_top_timed:")
    for item in aggregate_summary["median_top_timed"][:top_n]:
        print(f"  {item['name']} {float(item['total_ms']):.3f}ms")
    if aggregate_summary["median_top_scalars"]:
        print("median_top_scalars:")
        for item in aggregate_summary["median_top_scalars"][: min(top_n, 10)]:
            print(f"  {item['name']} {int(item['value'])}")
    if comparison:
        print(
            f"compare_duration_ms baseline={comparison['baseline_duration_ms']:.3f} "
            f"current={comparison['current_duration_ms']:.3f} "
            f"delta={comparison['delta_duration_ms']:+.3f}"
        )
        if comparison["timed_deltas"]:
            print("top_timed_deltas:")
            for item in comparison["timed_deltas"][:top_n]:
                print(
                    f"  {item['name']} current={item['current_ms']:.3f}ms "
                    f"baseline={item['baseline_ms']:.3f}ms delta={item['delta_ms']:+.3f}ms"
                )
        if comparison["scalar_deltas"]:
            print("top_scalar_deltas:")
            for item in comparison["scalar_deltas"][: min(top_n, 10)]:
                print(
                    f"  {item['name']} current={item['current_value']} "
                    f"baseline={item['baseline_value']} delta={item['delta_value']:+d}"
                )
    print(f"saved_csv={csv_path}")
    print(f"saved_summary={summary_path}")


def run_profile_series(
    cmd: list[str],
    cwd: Path,
    timeout_sec: int,
    rel_source: Path,
    repeat: int,
    warmup_runs: int,
    tmp_root: Path,
    env_overrides: dict[str, str],
    profile_mode: str,
) -> tuple[list[OracleRun], list[Path]]:
    if repeat <= 0:
        raise RuntimeError("--repeat must be >= 1")
    if warmup_runs < 0:
        raise RuntimeError("--warmup-runs must be >= 0")

    for warmup_index in range(warmup_runs):
        warmup_obj = tmp_root / rel_source.with_suffix(f".warmup{warmup_index + 1:02d}.o")
        warmup_csv = tmp_root / rel_source.with_suffix(f".warmup{warmup_index + 1:02d}.csv")
        warmup_obj.parent.mkdir(parents=True, exist_ok=True)
        warmup_csv.parent.mkdir(parents=True, exist_ok=True)
        warmup_result = run_cmd(
            cmd=cmd[:-1] + [str(warmup_obj)],
            cwd=cwd,
            timeout_sec=timeout_sec,
            profile_csv=warmup_csv,
            profile_mode=profile_mode,
            env_overrides=env_overrides,
        )
        if not warmup_result.ok:
            sys.stderr.write(warmup_result.stderr)
            if warmup_result.stdout:
                sys.stderr.write(warmup_result.stdout)
            raise RuntimeError(
                f"warmup profiled compile failed for {rel_source.as_posix()} "
                f"(returncode={warmup_result.returncode}, timed_out={warmup_result.timed_out})"
            )

    runs: list[OracleRun] = []
    temp_csv_paths: list[Path] = []
    for run_index in range(1, repeat + 1):
        obj_path = tmp_root / rel_source.with_suffix(f".run{run_index:02d}.o")
        profile_csv = tmp_root / rel_source.with_suffix(f".run{run_index:02d}.csv")
        obj_path.parent.mkdir(parents=True, exist_ok=True)
        profile_csv.parent.mkdir(parents=True, exist_ok=True)
        run_cmdline = cmd[:-1] + [str(obj_path)]
        result = run_cmd(
            cmd=run_cmdline,
            cwd=cwd,
            timeout_sec=timeout_sec,
            profile_csv=profile_csv,
            profile_mode=profile_mode,
            env_overrides=env_overrides,
        )
        if not result.ok:
            sys.stderr.write(result.stderr)
            if result.stdout:
                sys.stderr.write(result.stdout)
            raise RuntimeError(
                f"profiled compile failed for {rel_source.as_posix()} "
                f"(run={run_index}, returncode={result.returncode}, timed_out={result.timed_out})"
            )
        parsed_profile = parse_profile_csv(profile_csv)
        runs.append(OracleRun(run_index=run_index, result=result, parsed_profile=parsed_profile))
        temp_csv_paths.append(profile_csv)
    return runs, temp_csv_paths


def main() -> int:
    args = parse_args()
    try:
        env_overrides = parse_env_overrides(args.set_env)
        manifest = stage_a.load_manifest(args.manifest.resolve())
        project = stage_a.pick_project(manifest, args.project)
        project_root = stage_a.resolve_project_root(str(project["root"]))
        source = pick_source(project_root, project, args)
        rel_source = source.relative_to(project_root)
        stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
        timeout_sec = args.timeout_sec or int(stage_cfg.get("timeout_sec", DEFAULT_TIMEOUT_SEC))

        with tempfile.TemporaryDirectory(prefix="fisics-profile-oracle-") as tmp:
            tmp_root = Path(tmp)
            initial_obj = tmp_root / rel_source.with_suffix(".o")
            initial_obj.parent.mkdir(parents=True, exist_ok=True)
            cmd = build_fisics_cmd(project_root, project, source, initial_obj, args.extra_arg)
            runs, temp_csv_paths = run_profile_series(
                cmd=cmd,
                cwd=project_root,
                timeout_sec=timeout_sec,
                rel_source=rel_source,
                repeat=args.repeat,
                warmup_runs=args.warmup_runs,
                tmp_root=tmp_root,
                env_overrides=env_overrides,
                profile_mode=args.profile_mode,
            )

            csv_path, summary_path, existing_summary = save_artifacts(
                project_name=project["name"],
                rel_source=rel_source,
                cmd=cmd,
                profile_mode=args.profile_mode,
                runs=runs,
                temp_csv_paths=temp_csv_paths,
                artifact_label=args.artifact_label,
            )
            aggregate_summary = json.loads(summary_path.read_text(encoding="utf-8"))
            baseline_summary = read_summary_if_exists(args.compare_summary)
            if baseline_summary is None and existing_summary:
                baseline_summary = existing_summary
            comparison = build_comparison(aggregate_summary, baseline_summary) if baseline_summary else None

            print_summary(
                project_name=project["name"],
                rel_source=rel_source,
                aggregate_summary=aggregate_summary,
                csv_path=csv_path,
                summary_path=summary_path,
                top_n=args.top,
                comparison=comparison,
            )
            if args.keep_csv:
                kept_path = Path.cwd() / f"{rel_source.stem}.profile.csv"
                shutil.copyfile(csv_path, kept_path)
                print(f"kept_csv={kept_path}")
        return 0
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
