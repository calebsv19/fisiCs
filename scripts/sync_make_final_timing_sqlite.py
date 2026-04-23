#!/usr/bin/env python3
"""Mirror canonical make-final timing CSV into SQLite for query/rollup use."""

from __future__ import annotations

import argparse
import csv
import hashlib
import sqlite3
import sys
from datetime import datetime, timezone
from pathlib import Path


def parse_args() -> argparse.Namespace:
    root_dir = Path(__file__).resolve().parent.parent
    default_csv = root_dir / ".." / "docs" / "private_program_docs" / "fisiCs" / "audits" / "make_final_timing_log.csv"
    default_db = root_dir / ".." / "data" / "fisics_timing" / "make_final_timing.sqlite"

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", dest="csv_path", default=str(default_csv), help="Canonical timing CSV path")
    parser.add_argument("--db", dest="db_path", default=str(default_db), help="SQLite mirror path")
    return parser.parse_args()


def parse_int(value: str, field: str, line_no: int) -> int:
    try:
        return int(value)
    except ValueError as exc:
        raise ValueError(f"line {line_no}: invalid integer for {field}: {value!r}") from exc


def parse_float(value: str, field: str, line_no: int) -> float:
    try:
        return float(value)
    except ValueError as exc:
        raise ValueError(f"line {line_no}: invalid float for {field}: {value!r}") from exc


def normalize_row(raw: dict[str, str], line_no: int, source_csv_path: str) -> dict[str, object]:
    ordered_values = [
        raw["timestamp_utc"],
        raw["git_head"],
        raw["tag"],
        raw["runs"],
        raw["median_seconds"],
        raw["mean_seconds"],
        raw["min_seconds"],
        raw["max_seconds"],
        raw["pass_count"],
        raw["fail_count"],
        raw["skip_count"],
        raw["total_count"],
        raw["avg_seconds_per_test"],
        raw["last_run_status"],
    ]
    run_key = hashlib.sha1("|".join(ordered_values).encode("utf-8")).hexdigest()

    return {
        "run_key": run_key,
        "timestamp_utc": raw["timestamp_utc"],
        "git_head": raw["git_head"],
        "tag": raw["tag"],
        "runs": parse_int(raw["runs"], "runs", line_no),
        "median_seconds": parse_float(raw["median_seconds"], "median_seconds", line_no),
        "mean_seconds": parse_float(raw["mean_seconds"], "mean_seconds", line_no),
        "min_seconds": parse_float(raw["min_seconds"], "min_seconds", line_no),
        "max_seconds": parse_float(raw["max_seconds"], "max_seconds", line_no),
        "pass_count": parse_int(raw["pass_count"], "pass_count", line_no),
        "fail_count": parse_int(raw["fail_count"], "fail_count", line_no),
        "skip_count": parse_int(raw["skip_count"], "skip_count", line_no),
        "total_count": parse_int(raw["total_count"], "total_count", line_no),
        "avg_seconds_per_test": parse_float(raw["avg_seconds_per_test"], "avg_seconds_per_test", line_no),
        "last_run_status": raw["last_run_status"],
        "source_csv_path": source_csv_path,
        "imported_at_utc": datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
    }


def ensure_schema(conn: sqlite3.Connection) -> None:
    conn.executescript(
        """
        CREATE TABLE IF NOT EXISTS timing_runs (
            run_key TEXT PRIMARY KEY,
            timestamp_utc TEXT NOT NULL,
            git_head TEXT NOT NULL,
            tag TEXT NOT NULL,
            runs INTEGER NOT NULL,
            median_seconds REAL NOT NULL,
            mean_seconds REAL NOT NULL,
            min_seconds REAL NOT NULL,
            max_seconds REAL NOT NULL,
            pass_count INTEGER NOT NULL,
            fail_count INTEGER NOT NULL,
            skip_count INTEGER NOT NULL,
            total_count INTEGER NOT NULL,
            avg_seconds_per_test REAL NOT NULL,
            last_run_status TEXT NOT NULL,
            source_csv_path TEXT NOT NULL,
            imported_at_utc TEXT NOT NULL
        );

        CREATE INDEX IF NOT EXISTS idx_timing_runs_timestamp
            ON timing_runs(timestamp_utc DESC);

        CREATE INDEX IF NOT EXISTS idx_timing_runs_tag
            ON timing_runs(tag);
        """
    )


def main() -> int:
    args = parse_args()
    csv_path = Path(args.csv_path).expanduser().resolve()
    db_path = Path(args.db_path).expanduser().resolve()

    if not csv_path.exists():
        print(f"ERROR: CSV not found: {csv_path}", file=sys.stderr)
        return 2

    db_path.parent.mkdir(parents=True, exist_ok=True)

    inserted = 0
    updated = 0
    processed = 0

    with sqlite3.connect(db_path) as conn:
        ensure_schema(conn)

        with csv_path.open("r", encoding="utf-8", newline="") as handle:
            reader = csv.DictReader(handle)
            required = {
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
            }
            missing = required - set(reader.fieldnames or [])
            if missing:
                print(f"ERROR: CSV missing columns: {', '.join(sorted(missing))}", file=sys.stderr)
                return 2

            for line_no, raw in enumerate(reader, start=2):
                try:
                    normalized = normalize_row(raw, line_no, str(csv_path))
                except ValueError as exc:
                    print(f"ERROR: {exc}", file=sys.stderr)
                    return 2
                processed += 1

                exists = conn.execute(
                    "SELECT 1 FROM timing_runs WHERE run_key = ?",
                    (normalized["run_key"],),
                ).fetchone()

                conn.execute(
                    """
                    INSERT INTO timing_runs (
                        run_key, timestamp_utc, git_head, tag, runs,
                        median_seconds, mean_seconds, min_seconds, max_seconds,
                        pass_count, fail_count, skip_count, total_count,
                        avg_seconds_per_test, last_run_status,
                        source_csv_path, imported_at_utc
                    ) VALUES (
                        :run_key, :timestamp_utc, :git_head, :tag, :runs,
                        :median_seconds, :mean_seconds, :min_seconds, :max_seconds,
                        :pass_count, :fail_count, :skip_count, :total_count,
                        :avg_seconds_per_test, :last_run_status,
                        :source_csv_path, :imported_at_utc
                    )
                    ON CONFLICT(run_key) DO UPDATE SET
                        timestamp_utc = excluded.timestamp_utc,
                        git_head = excluded.git_head,
                        tag = excluded.tag,
                        runs = excluded.runs,
                        median_seconds = excluded.median_seconds,
                        mean_seconds = excluded.mean_seconds,
                        min_seconds = excluded.min_seconds,
                        max_seconds = excluded.max_seconds,
                        pass_count = excluded.pass_count,
                        fail_count = excluded.fail_count,
                        skip_count = excluded.skip_count,
                        total_count = excluded.total_count,
                        avg_seconds_per_test = excluded.avg_seconds_per_test,
                        last_run_status = excluded.last_run_status,
                        source_csv_path = excluded.source_csv_path,
                        imported_at_utc = excluded.imported_at_utc
                    """,
                    normalized,
                )

                if exists is None:
                    inserted += 1
                else:
                    updated += 1

        conn.commit()
        total_rows = conn.execute("SELECT COUNT(*) FROM timing_runs").fetchone()[0]

    print(f"CSV source: {csv_path}")
    print(f"SQLite mirror: {db_path}")
    print(f"rows_processed={processed} inserted={inserted} updated={updated} total_rows={total_rows}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
