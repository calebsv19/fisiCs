#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import shutil
import tempfile
import time
from dataclasses import asdict
from pathlib import Path
from typing import Any

import run_project_compile_tests as stage_a


SCRIPT_DIR = Path(__file__).resolve().parent
REAL_PROJECTS_ROOT = SCRIPT_DIR.parent
DEFAULT_REPORT_ROOT = REAL_PROJECTS_ROOT / "reports"
DEFAULT_STAGE_KEY = "A_tu_compile"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run one or more real-project translation units using the exact Stage A "
            "command shape, then rerun them with a selected forced-include pair "
            "removed so the dependency surface can be classified explicitly."
        )
    )
    parser.add_argument("--manifest", type=Path, default=stage_a.DEFAULT_MANIFEST)
    parser.add_argument("--project", required=True, help="Project name in manifest.")
    parser.add_argument("--stage", default=DEFAULT_STAGE_KEY, choices=[DEFAULT_STAGE_KEY])
    parser.add_argument("--source", default="", help="Exact project-relative source path.")
    parser.add_argument("--filter", default="", help="Substring filter on source relative path.")
    parser.add_argument("--limit", type=int, default=0, help="Limit number of selected sources.")
    parser.add_argument("--timeout-sec", type=int, default=0, help="Override stage timeout.")
    parser.add_argument(
        "--remove-include-substr",
        required=True,
        help="Substring used to select the -include header pair to remove.",
    )
    parser.add_argument(
        "--skip-fisics",
        action="store_true",
        help="Only run the clang baseline/ablation lanes.",
    )
    return parser.parse_args()


def pick_sources(project_root: Path, project: dict[str, Any], args: argparse.Namespace) -> list[Path]:
    globs = list(project.get("file_globs", []))
    excludes = list(project.get("exclude_globs", []))
    sources = stage_a.collect_sources(project_root, globs, excludes)
    if args.source:
        target = (project_root / args.source).resolve()
        if target not in sources:
            raise RuntimeError(f"source not found in project selection: {args.source}")
        return [target]
    if args.filter:
        sources = [path for path in sources if args.filter in path.relative_to(project_root).as_posix()]
    if args.limit > 0:
        sources = sources[: args.limit]
    if not sources:
        raise RuntimeError("no sources selected; provide --source or a matching --filter")
    return sources


def remove_forced_include_arg(extra_args: list[str], needle: str) -> tuple[list[str], list[str]]:
    stripped: list[str] = []
    removed: list[str] = []
    i = 0
    while i < len(extra_args):
        arg = extra_args[i]
        if arg == "-include" and (i + 1) < len(extra_args):
            value = extra_args[i + 1]
            if needle in value:
                removed.extend([arg, value])
                i += 2
                continue
        stripped.append(arg)
        i += 1
    return stripped, removed


def result_payload(result: stage_a.CommandResult) -> dict[str, Any]:
    payload = asdict(result)
    payload["stdout"] = result.stdout
    payload["stderr"] = result.stderr
    return payload


def summarize_row(
    rel_source: str,
    baseline_fisics: stage_a.CommandResult | None,
    ablated_fisics: stage_a.CommandResult | None,
    baseline_clang: stage_a.CommandResult,
    ablated_clang: stage_a.CommandResult,
) -> dict[str, Any]:
    if ablated_clang.ok:
        classification = "clang_passes_without_forced_include"
    elif baseline_clang.ok:
        classification = "clang_fails_without_forced_include"
    else:
        classification = "clang_baseline_not_green"

    fisics_classification = None
    if baseline_fisics is not None and ablated_fisics is not None:
        if ablated_fisics.ok:
            fisics_classification = "fisics_passes_without_forced_include"
        elif baseline_fisics.ok:
            fisics_classification = "fisics_fails_without_forced_include"
        else:
            fisics_classification = "fisics_baseline_not_green"

    return {
        "source": rel_source,
        "classification": classification,
        "fisics_classification": fisics_classification,
        "baseline_fisics_ms": None if baseline_fisics is None else baseline_fisics.duration_ms,
        "ablated_fisics_ms": None if ablated_fisics is None else ablated_fisics.duration_ms,
        "baseline_clang_ms": baseline_clang.duration_ms,
        "ablated_clang_ms": ablated_clang.duration_ms,
        "baseline_fisics_ok": None if baseline_fisics is None else baseline_fisics.ok,
        "ablated_fisics_ok": None if ablated_fisics is None else ablated_fisics.ok,
        "baseline_clang_ok": baseline_clang.ok,
        "ablated_clang_ok": ablated_clang.ok,
    }


def main() -> int:
    args = parse_args()
    manifest = stage_a.load_manifest(args.manifest.resolve())
    project = stage_a.pick_project(manifest, args.project)
    project_root = stage_a.resolve_project_root(str(project["root"]))
    stage_cfg = project.get("stages", {}).get(args.stage, {})
    timeout_sec = args.timeout_sec or int(stage_cfg.get("timeout_sec", 45))
    include_dirs = stage_a.merged_project_lists(project, "include_dirs")
    defines = stage_a.merged_project_lists(project, "defines")
    pp_flags = stage_a.render_preprocessor_flags(project_root, include_dirs, defines)
    fisics_extra_args = list(stage_cfg.get("fisics_extra_args", []))
    clang_extra_args = list(stage_cfg.get("clang_extra_args", []))

    ablated_fisics_extra_args, removed_fisics = remove_forced_include_arg(
        fisics_extra_args, args.remove_include_substr
    )
    ablated_clang_extra_args, removed_clang = remove_forced_include_arg(
        clang_extra_args, args.remove_include_substr
    )
    if not removed_clang and (args.skip_fisics or not removed_fisics):
        raise RuntimeError(
            f"did not find a matching -include pair for substring={args.remove_include_substr}"
        )

    sources = pick_sources(project_root, project, args)
    fisics_bin = (stage_a.FISICS_ROOT / "fisics").resolve()
    history_id = f"{time.strftime('%Y%m%dT%H%M%S')}_{time.time_ns() % 1_000_000_000:09d}"
    latest_report = DEFAULT_REPORT_ROOT / "latest" / f"{project['name']}_forced_include_census_latest.json"
    history_report = DEFAULT_REPORT_ROOT / "history" / f"{history_id}_{project['name']}_forced_include_census.json"

    rows: list[dict[str, Any]] = []
    with tempfile.TemporaryDirectory(prefix="fisics-include-census-") as tmp:
        tmp_root = Path(tmp)
        for source in sources:
            rel_source = source.relative_to(project_root).as_posix()
            clang_obj = tmp_root / "clang" / Path(rel_source).with_suffix(".o")
            clang_obj.parent.mkdir(parents=True, exist_ok=True)

            clang_baseline_cmd = (
                ["clang"] + clang_extra_args + pp_flags + ["-c", str(source), "-o", str(clang_obj)]
            )
            clang_ablated_obj = tmp_root / "clang_ablated" / Path(rel_source).with_suffix(".o")
            clang_ablated_obj.parent.mkdir(parents=True, exist_ok=True)
            clang_ablated_cmd = (
                ["clang"] + ablated_clang_extra_args + pp_flags + ["-c", str(source), "-o", str(clang_ablated_obj)]
            )
            clang_baseline = stage_a.run_cmd(clang_baseline_cmd, project_root, timeout_sec, False)
            clang_ablated = stage_a.run_cmd(clang_ablated_cmd, project_root, timeout_sec, False)

            fisics_baseline = None
            fisics_ablated = None
            fisics_baseline_cmd = None
            fisics_ablated_cmd = None
            if not args.skip_fisics:
                fisics_obj = tmp_root / "fisics" / Path(rel_source).with_suffix(".o")
                fisics_obj.parent.mkdir(parents=True, exist_ok=True)
                fisics_baseline_cmd = (
                    [str(fisics_bin)] + pp_flags + fisics_extra_args + ["-c", str(source), "-o", str(fisics_obj)]
                )
                fisics_ablated_obj = tmp_root / "fisics_ablated" / Path(rel_source).with_suffix(".o")
                fisics_ablated_obj.parent.mkdir(parents=True, exist_ok=True)
                fisics_ablated_cmd = (
                    [str(fisics_bin)]
                    + pp_flags
                    + ablated_fisics_extra_args
                    + ["-c", str(source), "-o", str(fisics_ablated_obj)]
                )
                fisics_baseline = stage_a.run_cmd(fisics_baseline_cmd, project_root, timeout_sec, False)
                fisics_ablated = stage_a.run_cmd(fisics_ablated_cmd, project_root, timeout_sec, False)

            row = summarize_row(
                rel_source=rel_source,
                baseline_fisics=fisics_baseline,
                ablated_fisics=fisics_ablated,
                baseline_clang=clang_baseline,
                ablated_clang=clang_ablated,
            )
            row["commands"] = {
                "clang_baseline": clang_baseline_cmd,
                "clang_ablated": clang_ablated_cmd,
                "fisics_baseline": fisics_baseline_cmd,
                "fisics_ablated": fisics_ablated_cmd,
            }
            row["results"] = {
                "clang_baseline": result_payload(clang_baseline),
                "clang_ablated": result_payload(clang_ablated),
                "fisics_baseline": None if fisics_baseline is None else result_payload(fisics_baseline),
                "fisics_ablated": None if fisics_ablated is None else result_payload(fisics_ablated),
            }
            rows.append(row)

    summary_counts: dict[str, int] = {}
    for row in rows:
        key = row["classification"]
        summary_counts[key] = summary_counts.get(key, 0) + 1

    report = {
        "project": project["name"],
        "stage": args.stage,
        "remove_include_substr": args.remove_include_substr,
        "removed_fisics_args": removed_fisics,
        "removed_clang_args": removed_clang,
        "source_count": len(rows),
        "summary_counts": summary_counts,
        "rows": rows,
    }

    latest_report.parent.mkdir(parents=True, exist_ok=True)
    history_report.parent.mkdir(parents=True, exist_ok=True)
    latest_report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    shutil.copy2(latest_report, history_report)

    print(
        f"project={project['name']} stage={args.stage} sources={len(rows)} "
        f"remove_include_substr={args.remove_include_substr}"
    )
    print(f"summary_counts={json.dumps(summary_counts, sort_keys=True)}")
    for row in rows:
        print(
            f"source={row['source']} class={row['classification']} "
            f"clang_ms={row['baseline_clang_ms']}->{row['ablated_clang_ms']} "
            f"fisics_ms={row['baseline_fisics_ms']}->{row['ablated_fisics_ms']}"
        )
    print(f"latest_report={latest_report}")
    print(f"history_report={history_report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
