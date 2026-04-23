#!/usr/bin/env python3
"""Render make-final timing rollup markdown from the SQLite mirror."""

from __future__ import annotations

import argparse
import sqlite3
from datetime import datetime, timedelta, timezone
from pathlib import Path
from statistics import median


def parse_args() -> argparse.Namespace:
    root_dir = Path(__file__).resolve().parent.parent
    default_csv = root_dir / ".." / "docs" / "private_program_docs" / "fisiCs" / "audits" / "make_final_timing_log.csv"
    default_db = root_dir / ".." / "data" / "fisics_timing" / "make_final_timing.sqlite"
    default_output = root_dir / ".." / "docs" / "private_program_docs" / "fisiCs" / "audits" / "make_final_timing_rollup.md"

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", dest="csv_path", default=str(default_csv), help="Canonical CSV path")
    parser.add_argument("--db", dest="db_path", default=str(default_db), help="SQLite mirror path")
    parser.add_argument("--output", dest="output_path", default=str(default_output), help="Markdown output path")
    return parser.parse_args()


def parse_ts(ts: str) -> datetime:
    return datetime.strptime(ts, "%Y-%m-%dT%H:%M:%SZ")


def format_float(value: float) -> str:
    return f"{value:.6f}"


def render_markdown(
    rows: list[dict[str, object]],
    csv_path: Path,
    db_path: Path,
) -> str:
    generated_at = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    lines: list[str] = []
    lines.append("# Make Final Timing Rollup")
    lines.append("")
    lines.append(f"Generated at: `{generated_at}`")
    lines.append(f"Canonical CSV: `{csv_path}`")
    lines.append(f"SQLite mirror: `{db_path}`")
    lines.append("")

    if not rows:
        lines.append("No timing rows are available in SQLite yet.")
        lines.append("")
        lines.append("Run `make final-timing-sync` first.")
        return "\n".join(lines) + "\n"

    latest = rows[0]
    last_10 = rows[:10]

    now = datetime.now(timezone.utc).replace(tzinfo=None)
    window_start = now - timedelta(days=7)
    window_7 = [row for row in rows if parse_ts(str(row["timestamp_utc"])) >= window_start]

    last_10_medians = [float(row["median_seconds"]) for row in last_10]
    last_10_avg_per_test = [float(row["avg_seconds_per_test"]) for row in last_10]

    lines.append("## Snapshot")
    lines.append("")
    lines.append(f"- latest timestamp: `{latest['timestamp_utc']}`")
    lines.append(f"- latest git head: `{latest['git_head']}`")
    lines.append(f"- latest tag: `{latest['tag']}`")
    lines.append(f"- latest median seconds: `{format_float(float(latest['median_seconds']))}`")
    lines.append(f"- latest total tests: `{int(latest['total_count'])}`")
    lines.append(f"- latest avg seconds/test: `{format_float(float(latest['avg_seconds_per_test']))}`")
    lines.append(f"- latest status: `{latest['last_run_status']}`")
    lines.append("")

    lines.append("## Rollups")
    lines.append("")
    lines.append(f"- last 10 captures count: `{len(last_10)}`")
    lines.append(f"- last 10 median of medians: `{format_float(median(last_10_medians))}`")
    lines.append(f"- last 10 median avg seconds/test: `{format_float(median(last_10_avg_per_test))}`")

    if window_7:
        window_7_medians = [float(row["median_seconds"]) for row in window_7]
        window_7_avg_per_test = [float(row["avg_seconds_per_test"]) for row in window_7]
        lines.append(f"- last 7 days captures count: `{len(window_7)}`")
        lines.append(f"- last 7 days median of medians: `{format_float(median(window_7_medians))}`")
        lines.append(f"- last 7 days median avg seconds/test: `{format_float(median(window_7_avg_per_test))}`")
    else:
        lines.append("- last 7 days captures count: `0`")
        lines.append("- last 7 days median of medians: `n/a`")
        lines.append("- last 7 days median avg seconds/test: `n/a`")

    lines.append("")
    lines.append("## Latest 10")
    lines.append("")
    lines.append("| timestamp_utc | git_head | tag | median_seconds | total_count | avg_seconds_per_test | status |")
    lines.append("|---|---|---|---:|---:|---:|---|")
    for row in last_10:
        lines.append(
            "| {timestamp_utc} | {git_head} | {tag} | {median_seconds} | {total_count} | {avg_seconds_per_test} | {last_run_status} |".format(
                timestamp_utc=row["timestamp_utc"],
                git_head=row["git_head"],
                tag=row["tag"],
                median_seconds=format_float(float(row["median_seconds"])),
                total_count=int(row["total_count"]),
                avg_seconds_per_test=format_float(float(row["avg_seconds_per_test"])),
                last_run_status=row["last_run_status"],
            )
        )

    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    csv_path = Path(args.csv_path).expanduser().resolve()
    db_path = Path(args.db_path).expanduser().resolve()
    output_path = Path(args.output_path).expanduser().resolve()

    if not db_path.exists():
        raise SystemExit(f"ERROR: SQLite DB not found: {db_path}")

    with sqlite3.connect(db_path) as conn:
        conn.row_factory = sqlite3.Row
        rows = conn.execute(
            """
            SELECT
                timestamp_utc,
                git_head,
                tag,
                median_seconds,
                total_count,
                avg_seconds_per_test,
                last_run_status
            FROM timing_runs
            ORDER BY timestamp_utc DESC
            """
        ).fetchall()

    output_path.parent.mkdir(parents=True, exist_ok=True)
    markdown = render_markdown([dict(row) for row in rows], csv_path, db_path)
    output_path.write_text(markdown, encoding="utf-8")

    print(f"Rendered rollup: {output_path}")
    print(f"rows_used={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
