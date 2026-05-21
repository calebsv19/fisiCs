#!/usr/bin/env python3
"""Shared helpers for fisiCs make-final timing and monitoring."""

from __future__ import annotations

import csv
import json
import math
import statistics
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


ROOT_DIR = Path(__file__).resolve().parent.parent
DEFAULT_LOG_PATH = (ROOT_DIR / ".." / "docs" / "private_program_docs" / "fisiCs" / "audits" / "make_final_timing_log.csv").resolve()
DEFAULT_NOTES_PATH = (ROOT_DIR / ".." / "docs" / "private_program_docs" / "fisiCs" / "audits" / "make_final_timing_notes.md").resolve()
DEFAULT_DB_PATH = (ROOT_DIR / ".." / "data" / "fisics_timing" / "make_final_timing.sqlite").resolve()
DEFAULT_ROLLUP_PATH = (ROOT_DIR / ".." / "docs" / "private_program_docs" / "fisiCs" / "audits" / "make_final_timing_rollup.md").resolve()
DEFAULT_BASELINE_PATH = (ROOT_DIR / ".." / "data" / "fisics_timing" / "make_final_timing_baseline.json").resolve()
DEFAULT_RUNS_ROOT = (ROOT_DIR / "build" / "make_final_runs").resolve()


CSV_COLUMNS = [
    "timestamp_utc",
    "git_head",
    "tag",
    "runs",
    "median_seconds",
    "mean_seconds",
    "min_seconds",
    "max_seconds",
    "pass_count",
    "fail_count",
    "skip_count",
    "total_count",
    "avg_seconds_per_test",
    "last_run_status",
]


@dataclass(frozen=True)
class TimingRow:
    timestamp_utc: str
    git_head: str
    tag: str
    runs: int
    median_seconds: float
    mean_seconds: float
    min_seconds: float
    max_seconds: float
    pass_count: int
    fail_count: int
    skip_count: int
    total_count: int
    avg_seconds_per_test: float
    last_run_status: str


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def ensure_csv_exists(csv_path: Path) -> None:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    if csv_path.exists():
        return
    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=CSV_COLUMNS)
        writer.writeheader()


def load_timing_rows(csv_path: Path) -> list[TimingRow]:
    if not csv_path.exists():
        return []

    rows: list[TimingRow] = []
    with csv_path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for raw in reader:
            rows.append(
                TimingRow(
                    timestamp_utc=raw["timestamp_utc"],
                    git_head=raw["git_head"],
                    tag=raw["tag"],
                    runs=int(raw["runs"]),
                    median_seconds=float(raw["median_seconds"]),
                    mean_seconds=float(raw["mean_seconds"]),
                    min_seconds=float(raw["min_seconds"]),
                    max_seconds=float(raw["max_seconds"]),
                    pass_count=int(raw["pass_count"]),
                    fail_count=int(raw["fail_count"]),
                    skip_count=int(raw["skip_count"]),
                    total_count=int(raw["total_count"]),
                    avg_seconds_per_test=float(raw["avg_seconds_per_test"]),
                    last_run_status=raw["last_run_status"],
                )
            )
    return rows


def successful_rows(rows: Iterable[TimingRow]) -> list[TimingRow]:
    return [row for row in rows if row.last_run_status == "ok"]


def _safe_median(values: list[float], limit: int) -> float | None:
    if not values:
        return None
    sample = values[-limit:]
    return float(statistics.median(sample))


def compute_baseline_payload(
    rows: list[TimingRow],
    *,
    min_first_check_seconds: int = 420,
    first_check_buffer_seconds: int = 30,
    stall_threshold_seconds: int = 900,
) -> dict[str, object]:
    ok_rows = successful_rows(rows)
    durations = [row.median_seconds for row in ok_rows]

    last_seconds = durations[-1] if durations else None
    median_3 = _safe_median(durations, 3)
    median_5 = _safe_median(durations, 5)

    if median_3 is not None:
        baseline_seconds = median_3
        baseline_source = "median_3"
    elif median_5 is not None:
        baseline_seconds = median_5
        baseline_source = "median_5"
    elif last_seconds is not None:
        baseline_seconds = last_seconds
        baseline_source = "last_success"
    else:
        baseline_seconds = float(min_first_check_seconds)
        baseline_source = "default_floor"

    recommended_first_check_seconds = max(
        int(min_first_check_seconds),
        int(math.ceil(baseline_seconds + first_check_buffer_seconds)),
    )

    last_row = ok_rows[-1] if ok_rows else None
    return {
        "schema": "fisics_make_final_timing_baseline_v1",
        "generated_at_utc": utc_now_iso(),
        "sample_count": len(ok_rows),
        "last_success_timestamp_utc": last_row.timestamp_utc if last_row else None,
        "last_success_git_head": last_row.git_head if last_row else None,
        "last_success_tag": last_row.tag if last_row else None,
        "last_seconds": last_seconds,
        "median_3_seconds": median_3,
        "median_5_seconds": median_5,
        "baseline_seconds": baseline_seconds,
        "baseline_source": baseline_source,
        "first_check_buffer_seconds": int(first_check_buffer_seconds),
        "min_first_check_seconds": int(min_first_check_seconds),
        "recommended_first_check_seconds": recommended_first_check_seconds,
        "stall_threshold_seconds": int(stall_threshold_seconds),
    }


def write_json_atomic(path: Path, payload: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp_path = path.with_suffix(path.suffix + ".tmp")
    tmp_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    tmp_path.replace(path)


def refresh_baseline_file(
    csv_path: Path,
    baseline_path: Path,
    *,
    min_first_check_seconds: int = 420,
    first_check_buffer_seconds: int = 60,
    stall_threshold_seconds: int = 900,
) -> dict[str, object]:
    rows = load_timing_rows(csv_path)
    payload = compute_baseline_payload(
        rows,
        min_first_check_seconds=min_first_check_seconds,
        first_check_buffer_seconds=first_check_buffer_seconds,
        stall_threshold_seconds=stall_threshold_seconds,
    )
    payload["csv_path"] = str(csv_path)
    write_json_atomic(baseline_path, payload)
    return payload


def append_timing_row(csv_path: Path, row: dict[str, object]) -> None:
    ensure_csv_exists(csv_path)
    with csv_path.open("a", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=CSV_COLUMNS)
        writer.writerow(row)


def append_note(notes_path: Path, note_line: str) -> None:
    notes_path.parent.mkdir(parents=True, exist_ok=True)
    if not notes_path.exists():
        notes_path.write_text(
            "# make final timing notes\n\n"
            "Use this file for notable context attached to timing captures.\n\n",
            encoding="utf-8",
        )
    with notes_path.open("a", encoding="utf-8") as handle:
        handle.write(note_line)


def clean_note(note: str) -> str:
    return " ".join(note.split())
