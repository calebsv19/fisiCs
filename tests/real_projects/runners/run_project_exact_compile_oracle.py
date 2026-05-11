#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

import run_project_compile_tests as stage_a


SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
DEFAULT_ARTIFACT_ROOT = REAL_PROJECTS_ROOT / "artifacts"
DEFAULT_STAGE_KEY = "A_tu_compile"
DEFAULT_TIMEOUT_SEC = 120
DEFAULT_REPEAT = 3
DEFAULT_CONTROL_BAND_RATIO = 1.35
DEFAULT_MAX_FISICS_SPREAD_RATIO = 1.20
DEFAULT_MAX_CLANG_SPREAD_RATIO = 1.20
DEFAULT_MAX_PAIR_RATIO_SPREAD = 1.20


@dataclass
class CommandResult:
    ok: bool
    returncode: int | None
    timed_out: bool
    duration_ms: int
    stdout: str
    stderr: str


@dataclass
class PairedRun:
    run_index: int
    clang: CommandResult | None
    fisics: CommandResult


@dataclass
class SentinelSpec:
    project_name: str
    project_root: Path
    source: Path
    source_rel: Path
    extra_args: list[str]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run the exact Stage-A TU compile command as the timing acceptance "
            "lane. By default this interleaves clang control compiles with "
            "fisiCs compiles, reports normalized control metrics, and marks "
            "noisy batches invalid for keep/drop decisions."
        )
    )
    parser.add_argument("--manifest", type=Path, default=stage_a.DEFAULT_MANIFEST)
    parser.add_argument("--project", required=True, help="Project name in manifest.")
    parser.add_argument("--source", default="", help="Exact project-relative source path.")
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
        "--repeat",
        type=int,
        default=DEFAULT_REPEAT,
        help="Number of measured paired runs to execute.",
    )
    parser.add_argument(
        "--warmup-runs",
        type=int,
        default=0,
        help="Number of unreported warmup paired runs before measured runs.",
    )
    parser.add_argument(
        "--extra-arg",
        action="append",
        default=[],
        help="Extra compiler argument to append to the Stage-A compile commands.",
    )
    parser.add_argument(
        "--set-env",
        action="append",
        default=[],
        help="Environment override in NAME=VALUE form. Repeat to set multiple values.",
    )
    parser.add_argument(
        "--artifact-label",
        default="",
        help="Optional suffix used to separate artifact sets for alternate command shapes.",
    )
    parser.add_argument(
        "--compare-summary",
        type=Path,
        default=None,
        help="Optional saved summary JSON to compare against.",
    )
    parser.add_argument(
        "--skip-clang-control",
        action="store_true",
        help="Disable the interleaved clang control lane. Do not use for canonical acceptance.",
    )
    parser.add_argument(
        "--control-band-ratio",
        type=float,
        default=DEFAULT_CONTROL_BAND_RATIO,
        help=(
            "Mark the batch noisy if the current clang median falls outside "
            "previous_clang_median / R .. previous_clang_median * R."
        ),
    )
    parser.add_argument(
        "--max-fisics-spread-ratio",
        type=float,
        default=DEFAULT_MAX_FISICS_SPREAD_RATIO,
        help="Mark the batch noisy if max(fisiCs)/min(fisiCs) exceeds this ratio.",
    )
    parser.add_argument(
        "--max-clang-spread-ratio",
        type=float,
        default=DEFAULT_MAX_CLANG_SPREAD_RATIO,
        help="Mark the batch noisy if max(clang)/min(clang) exceeds this ratio.",
    )
    parser.add_argument(
        "--max-pair-ratio-spread",
        type=float,
        default=DEFAULT_MAX_PAIR_RATIO_SPREAD,
        help=(
            "Mark the batch noisy if max(fisiCs/clangs)/min(fisiCs/clangs) exceeds this ratio."
        ),
    )
    parser.add_argument(
        "--sentinel-project",
        default="",
        help="Optional project name for a small preflight sentinel TU. Defaults to the main project.",
    )
    parser.add_argument(
        "--sentinel-source",
        default="",
        help="Exact project-relative source path for the sentinel TU.",
    )
    parser.add_argument(
        "--sentinel-filter",
        default="",
        help="Substring filter used to select the sentinel TU when --sentinel-source is not provided.",
    )
    parser.add_argument(
        "--sentinel-repeat",
        type=int,
        default=1,
        help="Number of measured paired runs for the sentinel preflight.",
    )
    parser.add_argument(
        "--sentinel-extra-arg",
        action="append",
        default=[],
        help="Extra compiler argument to append to the sentinel compile commands.",
    )
    parser.add_argument(
        "--sentinel-band-ratio",
        type=float,
        default=DEFAULT_CONTROL_BAND_RATIO,
        help=(
            "Mark the batch noisy if the sentinel clang median or sentinel fisics/clangs ratio "
            "falls outside the previous sentinel band by this ratio."
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


def pick_source_from_values(
    project_root: Path,
    project: dict[str, Any],
    source_value: str,
    filter_value: str,
) -> Path:
    globs = list(project.get("file_globs", []))
    excludes = list(project.get("exclude_globs", []))
    sources = stage_a.collect_sources(project_root, globs, excludes)
    if source_value:
        target = (project_root / source_value).resolve()
        if target not in sources:
            raise RuntimeError(f"source not found in project selection: {source_value}")
        return target
    if filter_value:
        matches = [path for path in sources if filter_value in path.relative_to(project_root).as_posix()]
        if not matches:
            raise RuntimeError(f"no sources matched filter={filter_value}")
        if len(matches) > 1:
            sample = "\n".join(f"  - {path.relative_to(project_root).as_posix()}" for path in matches[:10])
            raise RuntimeError(
                f"filter matched {len(matches)} sources; provide an exact source or narrow filter:\n{sample}"
            )
        return matches[0]
    raise RuntimeError("sentinel requested without --sentinel-source or --sentinel-filter")


def build_common_stage_a_parts(
    project_root: Path,
    project: dict[str, Any],
) -> tuple[list[str], list[str], list[str]]:
    stage_cfg = project.get("stages", {}).get(DEFAULT_STAGE_KEY, {})
    fisics_extra_args = list(stage_cfg.get("fisics_extra_args", []))
    clang_extra_args = list(stage_cfg.get("clang_extra_args", []))
    include_dirs = stage_a.merged_project_lists(project, "include_dirs")
    defines = stage_a.merged_project_lists(project, "defines")
    pp_flags = stage_a.render_preprocessor_flags(project_root, include_dirs, defines)
    return pp_flags, fisics_extra_args, clang_extra_args


def build_fisics_cmd(
    project_root: Path,
    project: dict[str, Any],
    source: Path,
    object_path: Path,
    extra_args: list[str],
) -> list[str]:
    pp_flags, fisics_extra_args, _ = build_common_stage_a_parts(project_root, project)
    fisics_bin = (stage_a.FISICS_ROOT / "fisics").resolve()
    return [str(fisics_bin)] + pp_flags + fisics_extra_args + list(extra_args) + ["-c", str(source), "-o", str(object_path)]


def build_clang_cmd(
    project_root: Path,
    project: dict[str, Any],
    source: Path,
    object_path: Path,
    extra_args: list[str],
) -> list[str]:
    pp_flags, _, clang_extra_args = build_common_stage_a_parts(project_root, project)
    return ["clang"] + clang_extra_args + pp_flags + list(extra_args) + ["-c", str(source), "-o", str(object_path)]


def build_sentinel_spec(
    manifest: dict[str, Any],
    args: argparse.Namespace,
) -> SentinelSpec | None:
    if not args.sentinel_source and not args.sentinel_filter and not args.sentinel_project:
        return None
    sentinel_project_name = args.sentinel_project or args.project
    sentinel_project = stage_a.pick_project(manifest, sentinel_project_name)
    sentinel_project_root = stage_a.resolve_project_root(sentinel_project["root"])
    sentinel_source = pick_source_from_values(
        sentinel_project_root,
        sentinel_project,
        args.sentinel_source,
        args.sentinel_filter,
    )
    return SentinelSpec(
        project_name=sentinel_project_name,
        project_root=sentinel_project_root,
        source=sentinel_source,
        source_rel=sentinel_source.relative_to(sentinel_project_root),
        extra_args=list(args.sentinel_extra_arg),
    )


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


def run_cmd(cmd: list[str], cwd: Path, timeout_sec: int, env_overrides: dict[str, str]) -> CommandResult:
    env = dict(os.environ)
    env.update(env_overrides)
    env.pop("FISICS_PROFILE", None)
    env.pop("FISICS_PROFILE_MODE", None)
    env.pop("FISICS_PROFILE_STREAM", None)
    env.pop("FISICS_PROFILE_PATH", None)
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


def artifact_base(project_name: str, source_rel: Path, artifact_label: str) -> Path:
    suffix = f".{artifact_label}" if artifact_label else ""
    return (
        DEFAULT_ARTIFACT_ROOT
        / "latest"
        / project_name
        / "exact_compile_oracle"
        / source_rel.parent
        / f"{source_rel.stem}{suffix}"
    )


def append_artifact_suffix(base: Path, suffix: str) -> Path:
    return base.parent / f"{base.name}{suffix}"


def history_summary_path(project_name: str) -> Path:
    stamp = time.strftime("%Y%m%dT%H%M%S")
    nanos = time.time_ns() % 1_000_000_000
    return (
        REAL_PROJECTS_ROOT
        / "reports"
        / "history"
        / f"{stamp}_{nanos:09d}_{project_name}_exact_compile_oracle.json"
    )


def safe_command_text(cmd: list[str]) -> str:
    return " ".join(cmd) + "\n"


def median_int(values: list[int]) -> int:
    return int(statistics.median(values))


def median_float(values: list[float]) -> float:
    return float(statistics.median(values))


def spread_ratio(values: list[int] | list[float]) -> float | None:
    if not values:
        return None
    lo = min(values)
    hi = max(values)
    if lo <= 0:
        return None
    return float(hi) / float(lo)


def load_previous_summary(path: Path | None) -> dict[str, Any] | None:
    if path is None or not path.exists():
        return None
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def previous_fisics_median(summary: dict[str, Any] | None) -> int | None:
    if not summary:
        return None
    value = summary.get("median_duration_ms")
    if isinstance(value, int):
        return value
    fisics = summary.get("fisics")
    if isinstance(fisics, dict):
        value = fisics.get("median_duration_ms")
        if isinstance(value, int):
            return value
    return None


def previous_clang_median(summary: dict[str, Any] | None) -> int | None:
    if not summary:
        return None
    clang = summary.get("clang")
    if isinstance(clang, dict):
        value = clang.get("median_duration_ms")
        if isinstance(value, int):
            return value
    return None


def previous_sentinel(summary: dict[str, Any] | None) -> dict[str, Any] | None:
    if not summary:
        return None
    sentinel = summary.get("sentinel")
    return sentinel if isinstance(sentinel, dict) else None


def previous_acceptance(summary: dict[str, Any] | None) -> dict[str, Any] | None:
    if not summary:
        return None
    acceptance = summary.get("acceptance")
    return acceptance if isinstance(acceptance, dict) else None


def safe_probe(cmd: list[str]) -> str | None:
    try:
        completed = subprocess.run(
            cmd,
            text=True,
            capture_output=True,
            timeout=3,
            check=False,
        )
    except Exception:
        return None
    text = completed.stdout.strip() or completed.stderr.strip()
    return text or None


def collect_system_metadata() -> dict[str, Any]:
    data: dict[str, Any] = {
        "captured_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "platform": platform.platform(),
        "python": sys.version.split()[0],
        "cpu_count": os.cpu_count(),
    }
    try:
        loadavg = os.getloadavg()
        data["loadavg"] = [round(loadavg[0], 3), round(loadavg[1], 3), round(loadavg[2], 3)]
    except (AttributeError, OSError):
        data["loadavg"] = None
    if sys.platform == "darwin":
        batt = safe_probe(["pmset", "-g", "batt"])
        if batt:
            power_source = "unknown"
            if "AC Power" in batt:
                power_source = "ac"
            elif "Battery Power" in batt:
                power_source = "battery"
            data["power_source"] = power_source
            data["battery_probe"] = batt
    return data


def compare_lines(previous: dict[str, Any] | None, fisics_median_ms: int, clang_median_ms: int | None) -> list[str]:
    lines: list[str] = []
    prev_fisics = previous_fisics_median(previous)
    if prev_fisics is not None:
        delta = fisics_median_ms - prev_fisics
        direction = "faster" if delta < 0 else "slower" if delta > 0 else "flat"
        lines.append(
            f"compare fisics median {prev_fisics} ms -> {fisics_median_ms} ms ({delta:+d} ms, {direction})"
        )
    prev_clang = previous_clang_median(previous)
    if prev_clang is not None and clang_median_ms is not None:
        delta = clang_median_ms - prev_clang
        direction = "faster" if delta < 0 else "slower" if delta > 0 else "flat"
        lines.append(
            f"compare clang median {prev_clang} ms -> {clang_median_ms} ms ({delta:+d} ms, {direction})"
        )
    return lines


def summarize_paired_runs(paired_runs: list[PairedRun]) -> dict[str, Any]:
    fisics_durations = [row.fisics.duration_ms for row in paired_runs]
    clang_durations = [row.clang.duration_ms for row in paired_runs if row.clang is not None]
    pair_ratios = [
        round(row.fisics.duration_ms / row.clang.duration_ms, 6)
        for row in paired_runs
        if row.clang is not None and row.clang.duration_ms > 0
    ]
    pair_deltas = [
        row.fisics.duration_ms - row.clang.duration_ms
        for row in paired_runs
        if row.clang is not None
    ]
    return {
        "fisics_durations": fisics_durations,
        "clang_durations": clang_durations,
        "pair_ratios": pair_ratios,
        "pair_deltas": pair_deltas,
        "fisics_median_ms": median_int(fisics_durations),
        "fisics_mean_ms": round(sum(fisics_durations) / len(fisics_durations), 3),
        "clang_median_ms": median_int(clang_durations) if clang_durations else None,
        "clang_mean_ms": round(sum(clang_durations) / len(clang_durations), 3) if clang_durations else None,
        "pair_ratio_median": median_float(pair_ratios) if pair_ratios else None,
        "pair_delta_median": median_int(pair_deltas) if pair_deltas else None,
    }


def runs_summary(paired_runs: list[PairedRun]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for row in paired_runs:
        run_summary: dict[str, Any] = {
            "run_index": row.run_index,
            "clang": None if row.clang is None else asdict(row.clang),
            "fisics": asdict(row.fisics),
        }
        if row.clang is not None and row.clang.duration_ms > 0:
            run_summary["fisics_over_clang_ratio"] = round(
                row.fisics.duration_ms / row.clang.duration_ms, 6
            )
            run_summary["fisics_minus_clang_ms"] = row.fisics.duration_ms - row.clang.duration_ms
        rows.append(run_summary)
    return rows


def run_paired_sequence(
    *,
    repeat: int,
    warmup_runs: int,
    timeout_sec: int,
    env_overrides: dict[str, str],
    clang_cmd: list[str] | None,
    fisics_cmd: list[str],
    label: str,
) -> list[PairedRun]:
    for _ in range(max(0, warmup_runs)):
        if clang_cmd is not None:
            warm_clang = run_cmd(clang_cmd, stage_a.FISICS_ROOT, timeout_sec, env_overrides)
            if not warm_clang.ok:
                sys.stderr.write(warm_clang.stdout)
                sys.stderr.write(warm_clang.stderr)
                print(
                    f"warmup_failed compiler=clang label={label} timed_out={warm_clang.timed_out} "
                    f"returncode={warm_clang.returncode}"
                )
                raise SystemExit(2)
        warm_fisics = run_cmd(fisics_cmd, stage_a.FISICS_ROOT, timeout_sec, env_overrides)
        if not warm_fisics.ok:
            sys.stderr.write(warm_fisics.stdout)
            sys.stderr.write(warm_fisics.stderr)
            print(
                f"warmup_failed compiler=fisics label={label} timed_out={warm_fisics.timed_out} "
                f"returncode={warm_fisics.returncode}"
            )
            raise SystemExit(2)

    paired_runs: list[PairedRun] = []
    for run_index in range(1, max(1, repeat) + 1):
        clang_result: CommandResult | None = None
        if clang_cmd is not None:
            clang_result = run_cmd(clang_cmd, stage_a.FISICS_ROOT, timeout_sec, env_overrides)
            if not clang_result.ok:
                sys.stderr.write(clang_result.stdout)
                sys.stderr.write(clang_result.stderr)
                print(
                    f"run_failed compiler=clang label={label} run={run_index} "
                    f"timed_out={clang_result.timed_out} returncode={clang_result.returncode}"
                )
                raise SystemExit(2)

        fisics_result = run_cmd(fisics_cmd, stage_a.FISICS_ROOT, timeout_sec, env_overrides)
        if not fisics_result.ok:
            sys.stderr.write(fisics_result.stdout)
            sys.stderr.write(fisics_result.stderr)
            print(
                f"run_failed compiler=fisics label={label} run={run_index} "
                f"timed_out={fisics_result.timed_out} returncode={fisics_result.returncode}"
            )
            raise SystemExit(2)
        paired_runs.append(PairedRun(run_index=run_index, clang=clang_result, fisics=fisics_result))
    return paired_runs


def evaluate_acceptance(
    args: argparse.Namespace,
    previous: dict[str, Any] | None,
    fisics_durations: list[int],
    clang_durations: list[int] | None,
    pair_ratios: list[float] | None,
    sentinel_summary: dict[str, Any] | None,
) -> dict[str, Any]:
    reasons: list[str] = []
    prev_acceptance = previous_acceptance(previous)
    previous_valid_for_acceptance = bool(prev_acceptance and prev_acceptance.get("valid_for_acceptance"))
    fisics_spread = spread_ratio(fisics_durations)
    clang_spread = spread_ratio(clang_durations) if clang_durations else None
    pair_ratio_spread = spread_ratio(pair_ratios) if pair_ratios else None
    if fisics_spread is not None and fisics_spread > args.max_fisics_spread_ratio:
        reasons.append(
            f"fisics_spread_ratio={fisics_spread:.3f} exceeds {args.max_fisics_spread_ratio:.3f}"
        )
    if clang_spread is not None and clang_spread > args.max_clang_spread_ratio:
        reasons.append(
            f"clang_spread_ratio={clang_spread:.3f} exceeds {args.max_clang_spread_ratio:.3f}"
        )
    if pair_ratio_spread is not None and pair_ratio_spread > args.max_pair_ratio_spread:
        reasons.append(
            f"pair_ratio_spread={pair_ratio_spread:.3f} exceeds {args.max_pair_ratio_spread:.3f}"
        )

    control_status = "not_run"
    baseline: dict[str, Any] | None = None
    prev_clang = previous_clang_median(previous) if previous_valid_for_acceptance else None
    if clang_durations:
        control_status = "no_baseline"
        clang_median = median_int(clang_durations)
        if prev_clang is not None and prev_clang > 0:
            low = prev_clang / args.control_band_ratio
            high = prev_clang * args.control_band_ratio
            baseline = {
                "previous_clang_median_ms": prev_clang,
                "allowed_min_ms": round(low, 3),
                "allowed_max_ms": round(high, 3),
                "current_clang_median_ms": clang_median,
            }
            if clang_median < low or clang_median > high:
                reasons.append(
                    f"clang_control_out_of_band current={clang_median}ms baseline={prev_clang}ms band={low:.1f}..{high:.1f}ms"
                )
                control_status = "out_of_band"
            else:
                control_status = "in_band"

    sentinel_status = "not_run"
    sentinel_baseline: dict[str, Any] | None = None
    prev_sentinel = previous_sentinel(previous) if previous_valid_for_acceptance else None
    if sentinel_summary is not None:
        sentinel_status = "no_baseline"
        sentinel_clang = sentinel_summary.get("clang_median_ms")
        sentinel_ratio = sentinel_summary.get("pair_ratio_median")
        if isinstance(prev_sentinel, dict) and prev_acceptance and prev_acceptance.get("sentinel_status") == "in_band":
            prev_sentinel_clang = prev_sentinel.get("clang_median_ms")
            prev_sentinel_ratio = prev_sentinel.get("pair_ratio_median")
            sentinel_baseline = {
                "previous_clang_median_ms": prev_sentinel_clang,
                "previous_pair_ratio_median": prev_sentinel_ratio,
                "current_clang_median_ms": sentinel_clang,
                "current_pair_ratio_median": sentinel_ratio,
            }
            reasons_local: list[str] = []
            if isinstance(prev_sentinel_clang, int) and prev_sentinel_clang > 0 and isinstance(sentinel_clang, int):
                low = prev_sentinel_clang / args.sentinel_band_ratio
                high = prev_sentinel_clang * args.sentinel_band_ratio
                sentinel_baseline["allowed_clang_min_ms"] = round(low, 3)
                sentinel_baseline["allowed_clang_max_ms"] = round(high, 3)
                if sentinel_clang < low or sentinel_clang > high:
                    reasons_local.append(
                        f"sentinel_clang_out_of_band current={sentinel_clang}ms baseline={prev_sentinel_clang}ms band={low:.1f}..{high:.1f}ms"
                    )
            if isinstance(prev_sentinel_ratio, (int, float)) and prev_sentinel_ratio > 0 and isinstance(sentinel_ratio, float):
                low_ratio = prev_sentinel_ratio / args.sentinel_band_ratio
                high_ratio = prev_sentinel_ratio * args.sentinel_band_ratio
                sentinel_baseline["allowed_ratio_min"] = round(low_ratio, 6)
                sentinel_baseline["allowed_ratio_max"] = round(high_ratio, 6)
                if sentinel_ratio < low_ratio or sentinel_ratio > high_ratio:
                    reasons_local.append(
                        f"sentinel_ratio_out_of_band current={sentinel_ratio:.6f} baseline={prev_sentinel_ratio:.6f} band={low_ratio:.6f}..{high_ratio:.6f}"
                    )
            if reasons_local:
                reasons.extend(reasons_local)
                sentinel_status = "out_of_band"
            else:
                sentinel_status = "in_band"

    if args.skip_clang_control:
        status = "fisics_only"
        valid_for_acceptance = False
        reasons.append("clang_control_disabled")
    elif reasons:
        status = "noisy"
        valid_for_acceptance = False
    elif clang_durations:
        if prev_clang is None:
            status = "provisional"
            valid_for_acceptance = True
        else:
            status = "valid"
            valid_for_acceptance = True
    else:
        status = "provisional"
        valid_for_acceptance = True

    return {
        "status": status,
        "valid_for_acceptance": valid_for_acceptance,
        "reasons": reasons,
        "thresholds": {
            "control_band_ratio": args.control_band_ratio,
            "max_fisics_spread_ratio": args.max_fisics_spread_ratio,
            "max_clang_spread_ratio": args.max_clang_spread_ratio,
            "max_pair_ratio_spread": args.max_pair_ratio_spread,
        },
        "observed": {
            "fisics_spread_ratio": fisics_spread,
            "clang_spread_ratio": clang_spread,
            "pair_ratio_spread": pair_ratio_spread,
        },
        "control_baseline": baseline,
        "control_status": control_status,
        "sentinel_status": sentinel_status,
        "sentinel_baseline": sentinel_baseline,
    }


def main() -> int:
    args = parse_args()
    manifest = stage_a.load_manifest(args.manifest)
    project = stage_a.pick_project(manifest, args.project)
    project_name = project["name"]
    project_root = stage_a.resolve_project_root(project["root"])
    source = pick_source(project_root, project, args)
    source_rel = source.relative_to(project_root)
    sentinel_spec = build_sentinel_spec(manifest, args)
    timeout_sec = args.timeout_sec or int(
        project.get("stages", {}).get(DEFAULT_STAGE_KEY, {}).get("timeout_sec", DEFAULT_TIMEOUT_SEC)
    )
    env_overrides = parse_env_overrides(args.set_env)

    base = artifact_base(project_name, source_rel, args.artifact_label)
    summary_path = append_artifact_suffix(base, ".summary.json")
    fisics_cmd_path = append_artifact_suffix(base, ".fisics.cmd.txt")
    clang_cmd_path = append_artifact_suffix(base, ".clang.cmd.txt")
    fisics_stdout_path = append_artifact_suffix(base, ".fisics.stdout.txt")
    fisics_stderr_path = append_artifact_suffix(base, ".fisics.stderr.txt")
    clang_stdout_path = append_artifact_suffix(base, ".clang.stdout.txt")
    clang_stderr_path = append_artifact_suffix(base, ".clang.stderr.txt")
    for path in (
        summary_path,
        fisics_cmd_path,
        clang_cmd_path,
        fisics_stdout_path,
        fisics_stderr_path,
        clang_stdout_path,
        clang_stderr_path,
    ):
        path.parent.mkdir(parents=True, exist_ok=True)

    compare_path = args.compare_summary
    if compare_path is None and summary_path.exists():
        compare_path = summary_path
    previous = load_previous_summary(compare_path)

    system_metadata_start = collect_system_metadata()

    with tempfile.TemporaryDirectory(prefix="fisics-exact-compile-") as tmpdir:
        sentinel_summary: dict[str, Any] | None = None
        sentinel_runs: list[PairedRun] = []

        if sentinel_spec is not None:
            sentinel_fisics_object_path = Path(tmpdir) / "sentinel-fisics" / sentinel_spec.source_rel.with_suffix(".o")
            sentinel_fisics_object_path.parent.mkdir(parents=True, exist_ok=True)
            sentinel_fisics_cmd = build_fisics_cmd(
                sentinel_spec.project_root,
                stage_a.pick_project(manifest, sentinel_spec.project_name),
                sentinel_spec.source,
                sentinel_fisics_object_path,
                sentinel_spec.extra_args,
            )
            sentinel_clang_cmd: list[str] | None = None
            if not args.skip_clang_control:
                sentinel_clang_object_path = Path(tmpdir) / "sentinel-clang" / sentinel_spec.source_rel.with_suffix(".o")
                sentinel_clang_object_path.parent.mkdir(parents=True, exist_ok=True)
                sentinel_clang_cmd = build_clang_cmd(
                    sentinel_spec.project_root,
                    stage_a.pick_project(manifest, sentinel_spec.project_name),
                    sentinel_spec.source,
                    sentinel_clang_object_path,
                    sentinel_spec.extra_args,
                )
            sentinel_runs = run_paired_sequence(
                repeat=max(1, args.sentinel_repeat),
                warmup_runs=0,
                timeout_sec=timeout_sec,
                env_overrides=env_overrides,
                clang_cmd=sentinel_clang_cmd,
                fisics_cmd=sentinel_fisics_cmd,
                label=f"sentinel:{sentinel_spec.project_name}:{sentinel_spec.source_rel.as_posix()}",
            )
            sentinel_stats = summarize_paired_runs(sentinel_runs)
            sentinel_summary = {
                "project": sentinel_spec.project_name,
                "source": sentinel_spec.source_rel.as_posix(),
                "extra_args": sentinel_spec.extra_args,
                "fisics_command": sentinel_fisics_cmd,
                "clang_command": sentinel_clang_cmd,
                "repeat": len(sentinel_runs),
                "fisics_durations_ms": sentinel_stats["fisics_durations"],
                "fisics_median_ms": sentinel_stats["fisics_median_ms"],
                "clang_durations_ms": sentinel_stats["clang_durations"],
                "clang_median_ms": sentinel_stats["clang_median_ms"],
                "pair_ratios": sentinel_stats["pair_ratios"],
                "pair_ratio_median": sentinel_stats["pair_ratio_median"],
                "pair_deltas_ms": sentinel_stats["pair_deltas"],
                "pair_delta_median_ms": sentinel_stats["pair_delta_median"],
                "runs": runs_summary(sentinel_runs),
            }

        fisics_object_path = Path(tmpdir) / "fisics" / source_rel.with_suffix(".o")
        fisics_object_path.parent.mkdir(parents=True, exist_ok=True)
        fisics_cmd = build_fisics_cmd(project_root, project, source, fisics_object_path, args.extra_arg)

        clang_cmd: list[str] | None = None
        if not args.skip_clang_control:
            clang_object_path = Path(tmpdir) / "clang" / source_rel.with_suffix(".o")
            clang_object_path.parent.mkdir(parents=True, exist_ok=True)
            clang_cmd = build_clang_cmd(project_root, project, source, clang_object_path, args.extra_arg)

        paired_runs = run_paired_sequence(
            repeat=max(1, args.repeat),
            warmup_runs=max(0, args.warmup_runs),
            timeout_sec=timeout_sec,
            env_overrides=env_overrides,
            clang_cmd=clang_cmd,
            fisics_cmd=fisics_cmd,
            label=f"main:{project_name}:{source_rel.as_posix()}",
        )

    stats = summarize_paired_runs(paired_runs)
    fisics_durations = stats["fisics_durations"]
    clang_durations = stats["clang_durations"]
    pair_ratios = stats["pair_ratios"]
    pair_deltas = stats["pair_deltas"]
    fisics_median_ms = stats["fisics_median_ms"]
    fisics_mean_ms = stats["fisics_mean_ms"]
    clang_median_ms = stats["clang_median_ms"]
    clang_mean_ms = stats["clang_mean_ms"]
    pair_ratio_median = stats["pair_ratio_median"]
    pair_delta_median = stats["pair_delta_median"]

    system_metadata_end = collect_system_metadata()
    acceptance = evaluate_acceptance(
        args,
        previous,
        fisics_durations,
        clang_durations or None,
        pair_ratios or None,
        sentinel_summary,
    )

    summary: dict[str, Any] = {
        "summary_version": 2,
        "project": project_name,
        "source": source_rel.as_posix(),
        "command": fisics_cmd,
        "clang_command": clang_cmd,
        "env_overrides": env_overrides,
        "repeat": len(paired_runs),
        "warmup_runs": max(0, args.warmup_runs),
        "measurement_order": ["clang", "fisics"] if clang_cmd is not None else ["fisics"],
        "artifact_label": args.artifact_label,
        "durations_ms": fisics_durations,
        "median_duration_ms": fisics_median_ms,
        "min_duration_ms": min(fisics_durations),
        "max_duration_ms": max(fisics_durations),
        "mean_duration_ms": fisics_mean_ms,
        "fisics": {
            "durations_ms": fisics_durations,
            "median_duration_ms": fisics_median_ms,
            "min_duration_ms": min(fisics_durations),
            "max_duration_ms": max(fisics_durations),
            "mean_duration_ms": fisics_mean_ms,
        },
        "clang": None if not clang_durations else {
            "durations_ms": clang_durations,
            "median_duration_ms": clang_median_ms,
            "min_duration_ms": min(clang_durations),
            "max_duration_ms": max(clang_durations),
            "mean_duration_ms": clang_mean_ms,
        },
        "paired_metrics": None if not pair_ratios else {
            "fisics_over_clang_ratios": pair_ratios,
            "median_fisics_over_clang_ratio": pair_ratio_median,
            "fisics_minus_clang_ms": pair_deltas,
            "median_fisics_minus_clang_ms": pair_delta_median,
        },
        "acceptance": acceptance,
        "system_metadata": {
            "start": system_metadata_start,
            "end": system_metadata_end,
        },
        "sentinel": sentinel_summary,
        "compare_summary_path": None if compare_path is None else str(compare_path),
        "runs": runs_summary(paired_runs),
    }

    with summary_path.open("w", encoding="utf-8") as f:
        json.dump(summary, f, indent=2)
        f.write("\n")
    with fisics_cmd_path.open("w", encoding="utf-8") as f:
        f.write(safe_command_text(fisics_cmd))
    if clang_cmd is not None:
        with clang_cmd_path.open("w", encoding="utf-8") as f:
            f.write(safe_command_text(clang_cmd))
    with fisics_stdout_path.open("w", encoding="utf-8") as f:
        for row in paired_runs:
            f.write(f"=== run {row.run_index} stdout ===\n")
            f.write(row.fisics.stdout)
    with fisics_stderr_path.open("w", encoding="utf-8") as f:
        for row in paired_runs:
            f.write(f"=== run {row.run_index} stderr ===\n")
            f.write(row.fisics.stderr)
    if clang_cmd is not None:
        with clang_stdout_path.open("w", encoding="utf-8") as f:
            for row in paired_runs:
                if row.clang is None:
                    continue
                f.write(f"=== run {row.run_index} stdout ===\n")
                f.write(row.clang.stdout)
        with clang_stderr_path.open("w", encoding="utf-8") as f:
            for row in paired_runs:
                if row.clang is None:
                    continue
                f.write(f"=== run {row.run_index} stderr ===\n")
                f.write(row.clang.stderr)

    history_path = history_summary_path(project_name)
    history_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(summary_path, history_path)

    print(f"project={project_name} source={source_rel.as_posix()}")
    print(
        f"fisics_repeat={len(fisics_durations)} durations_ms={fisics_durations} "
        f"median_duration_ms={fisics_median_ms} min_duration_ms={min(fisics_durations)} "
        f"max_duration_ms={max(fisics_durations)} mean_duration_ms={fisics_mean_ms}"
    )
    if clang_durations:
        print(
            f"clang_repeat={len(clang_durations)} durations_ms={clang_durations} "
            f"median_duration_ms={clang_median_ms} min_duration_ms={min(clang_durations)} "
            f"max_duration_ms={max(clang_durations)} mean_duration_ms={clang_mean_ms}"
        )
    if pair_ratios:
        print(
            f"paired median_fisics_over_clang_ratio={pair_ratio_median:.6f} "
            f"median_fisics_minus_clang_ms={pair_delta_median} "
            f"ratios={pair_ratios} deltas_ms={pair_deltas}"
        )
    if sentinel_summary is not None:
        print(
            f"sentinel project={sentinel_summary['project']} source={sentinel_summary['source']} "
            f"fisics_median_ms={sentinel_summary['fisics_median_ms']} "
            f"clang_median_ms={sentinel_summary['clang_median_ms']} "
            f"pair_ratio_median={sentinel_summary['pair_ratio_median']}"
        )
    print(
        f"acceptance_status={acceptance['status']} "
        f"valid_for_acceptance={int(acceptance['valid_for_acceptance'])}"
    )
    if acceptance["reasons"]:
        print(f"acceptance_reasons={acceptance['reasons']}")
    print(f"saved_summary={summary_path}")
    print(f"saved_fisics_cmd={fisics_cmd_path}")
    if clang_cmd is not None:
        print(f"saved_clang_cmd={clang_cmd_path}")
    print(f"history_summary={history_path}")
    for line in compare_lines(previous, fisics_median_ms, clang_median_ms):
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
