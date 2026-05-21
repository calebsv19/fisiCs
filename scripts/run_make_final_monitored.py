#!/usr/bin/env python3
"""Run make final under a live status contract backed by timing history."""

from __future__ import annotations

import argparse
import math
import os
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

from make_final_timing_common import DEFAULT_BASELINE_PATH
from make_final_timing_common import DEFAULT_LOG_PATH
from make_final_timing_common import DEFAULT_NOTES_PATH
from make_final_timing_common import DEFAULT_RUNS_ROOT
from make_final_timing_common import ROOT_DIR
from make_final_timing_common import append_note
from make_final_timing_common import append_timing_row
from make_final_timing_common import clean_note
from make_final_timing_common import compute_baseline_payload
from make_final_timing_common import load_timing_rows
from make_final_timing_common import refresh_baseline_file
from make_final_timing_common import utc_now_iso
from make_final_timing_common import write_json_atomic


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runs", type=int, default=int(os.environ.get("FINAL_TIMING_RUNS", "1")), help="Number of sequential broad runs to execute")
    parser.add_argument("--csv", dest="csv_path", default=str(Path(os.environ.get("FINAL_TIMING_LOG", DEFAULT_LOG_PATH)).expanduser()), help="Canonical timing CSV path")
    parser.add_argument("--notes", dest="notes_path", default=str(Path(os.environ.get("FINAL_TIMING_NOTES", DEFAULT_NOTES_PATH)).expanduser()), help="Optional timing notes markdown path")
    parser.add_argument("--baseline-json", dest="baseline_path", default=str(DEFAULT_BASELINE_PATH), help="Derived timing baseline JSON path")
    parser.add_argument("--runs-root", dest="runs_root", default=str(DEFAULT_RUNS_ROOT), help="Directory for live run state and logs")
    parser.add_argument("--tag", default=os.environ.get("FINAL_TIMING_TAG", "manual"), help="Timing tag for successful captures")
    parser.add_argument("--note", default=os.environ.get("FINAL_TIMING_NOTE", ""), help="Optional note appended after a successful capture")
    parser.add_argument("--poll-seconds", type=int, default=60, help="Polling cadence after first audit")
    parser.add_argument("--min-first-check-seconds", type=int, default=420, help="Minimum delay before the first audit")
    parser.add_argument("--first-check-buffer-seconds", type=int, default=30, help="Buffer added to the timing baseline before the first audit")
    parser.add_argument("--stall-seconds", type=int, default=900, help="Observational no-output stall threshold")
    parser.add_argument("--label", default="make final", help="Human-readable command label")
    parser.add_argument("--no-record-history", action="store_true", help="Skip CSV and notes updates even on success")
    parser.add_argument(
        "command",
        nargs=argparse.REMAINDER,
        help="Optional command override after '--'. Defaults to `make -C <repo>/fisiCs final`.",
    )
    return parser.parse_args()


def default_command() -> list[str]:
    return ["make", "-C", str(ROOT_DIR), "final"]


def git_head() -> str:
    proc = subprocess.run(
        ["git", "rev-parse", "--short", "HEAD"],
        cwd=ROOT_DIR,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
        encoding="utf-8",
    )
    return proc.stdout.strip() or "unknown"


def run_id_now() -> str:
    return datetime.now(timezone.utc).strftime("mf_%Y%m%dT%H%M%SZ") + f"_{os.getpid()}"


def inspect_log(log_path: Path) -> dict[str, object]:
    if not log_path.exists():
        return {
            "size_bytes": 0,
            "line_count": 0,
            "last_output_at_epoch": None,
            "last_output_at_utc": None,
            "tail_line": "",
            "pass_count": 0,
            "fail_count": 0,
            "skip_count": 0,
        }

    stat = log_path.stat()
    text = log_path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    tail_line = ""
    for line in reversed(lines):
        stripped = line.strip()
        if stripped:
            tail_line = stripped
            break

    return {
        "size_bytes": stat.st_size,
        "line_count": len(lines),
        "last_output_at_epoch": stat.st_mtime,
        "last_output_at_utc": datetime.fromtimestamp(stat.st_mtime, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "tail_line": tail_line,
        "pass_count": sum(1 for line in lines if line.startswith("PASS ")),
        "fail_count": sum(1 for line in lines if line.startswith("FAIL ")),
        "skip_count": sum(1 for line in lines if line.startswith("SKIP ")),
    }


def duration_stats(values: list[float]) -> tuple[float, float, float, float]:
    values = list(values)
    values_sorted = sorted(values)
    if len(values_sorted) % 2 == 1:
        median = values_sorted[len(values_sorted) // 2]
    else:
        mid = len(values_sorted) // 2
        median = (values_sorted[mid - 1] + values_sorted[mid]) / 2.0
    mean = sum(values_sorted) / len(values_sorted)
    return median, mean, values_sorted[0], values_sorted[-1]


def build_status_payload(
    *,
    run_id: str,
    state: str,
    command: list[str],
    label: str,
    git_rev: str,
    baseline_payload: dict[str, object],
    status_path: Path,
    log_path: Path,
    started_at_epoch: float,
    started_at_utc: str,
    current_run_index: int,
    runs_total: int,
    runs_completed: int,
    next_audit_epoch: float | None,
    first_audit_epoch: float | None,
    proc: subprocess.Popen[bytes] | None,
    log_info: dict[str, object],
    exit_code: int | None,
    finished_at_utc: str | None = None,
    failure_reason: str | None = None,
) -> dict[str, object]:
    now_epoch = time.time()
    last_output_at_epoch = log_info["last_output_at_epoch"]
    stale_seconds = None
    if last_output_at_epoch is not None:
        stale_seconds = max(0, int(now_epoch - float(last_output_at_epoch)))

    return {
        "schema": "fisics_make_final_status_v1",
        "run_id": run_id,
        "state": state,
        "label": label,
        "command": command,
        "git_head": git_rev,
        "pid": proc.pid if proc is not None else None,
        "started_at_utc": started_at_utc,
        "updated_at_utc": utc_now_iso(),
        "finished_at_utc": finished_at_utc,
        "elapsed_seconds": round(max(0.0, time.monotonic() - started_at_epoch), 6),
        "current_run_index": current_run_index,
        "runs_total": runs_total,
        "runs_completed": runs_completed,
        "baseline_seconds": baseline_payload["baseline_seconds"],
        "baseline_source": baseline_payload["baseline_source"],
        "recommended_first_check_seconds": baseline_payload["recommended_first_check_seconds"],
        "first_audit_at_utc": (
            datetime.fromtimestamp(first_audit_epoch, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            if first_audit_epoch is not None
            else None
        ),
        "next_audit_at_utc": (
            datetime.fromtimestamp(next_audit_epoch, timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            if next_audit_epoch is not None
            else None
        ),
        "stall_threshold_seconds": baseline_payload["stall_threshold_seconds"],
        "last_output_at_utc": log_info["last_output_at_utc"],
        "seconds_since_output": stale_seconds,
        "last_output_line": log_info["line_count"],
        "last_output_tail": log_info["tail_line"],
        "log_size_bytes": log_info["size_bytes"],
        "pass_count": log_info["pass_count"],
        "fail_count": log_info["fail_count"],
        "skip_count": log_info["skip_count"],
        "log_path": str(log_path),
        "status_path": str(status_path),
        "exit_code": exit_code,
        "failure_reason": failure_reason,
    }


def write_status(path: Path, payload: dict[str, object]) -> None:
    write_json_atomic(path, payload)


def run_monitored_attempt(
    *,
    run_id: str,
    attempt_index: int,
    runs_total: int,
    command: list[str],
    label: str,
    git_rev: str,
    baseline_payload: dict[str, object],
    run_root: Path,
    status_path: Path,
    poll_seconds: int,
    stall_seconds: int,
) -> dict[str, object]:
    log_path = run_root / f"run_{attempt_index:02d}.stdout.log"
    first_audit_delay = int(baseline_payload["recommended_first_check_seconds"])
    first_audit_epoch = time.time() + first_audit_delay
    next_audit_epoch = first_audit_epoch
    started_wall_utc = utc_now_iso()
    started_monotonic = time.monotonic()

    with log_path.open("wb") as log_handle:
        proc = subprocess.Popen(
            command,
            cwd=ROOT_DIR,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
        )

        log_info = inspect_log(log_path)
        write_status(
            status_path,
            build_status_payload(
                run_id=run_id,
                state="starting",
                command=command,
                label=label,
                git_rev=git_rev,
                baseline_payload=baseline_payload,
                status_path=status_path,
                log_path=log_path,
                started_at_epoch=started_monotonic,
                started_at_utc=started_wall_utc,
                current_run_index=attempt_index,
                runs_total=runs_total,
                runs_completed=attempt_index - 1,
                next_audit_epoch=next_audit_epoch,
                first_audit_epoch=first_audit_epoch,
                proc=proc,
                log_info=log_info,
                exit_code=None,
            ),
        )

        while True:
            now = time.time()
            wait_seconds = max(0.0, next_audit_epoch - now)
            try:
                exit_code = proc.wait(timeout=wait_seconds)
                break
            except subprocess.TimeoutExpired:
                log_info = inspect_log(log_path)
                state = "running"
                last_output_at = log_info["last_output_at_epoch"]
                if last_output_at is not None and (time.time() - float(last_output_at)) >= stall_seconds:
                    state = "stalled"

                write_status(
                    status_path,
                    build_status_payload(
                        run_id=run_id,
                        state=state,
                        command=command,
                        label=label,
                        git_rev=git_rev,
                        baseline_payload=baseline_payload,
                        status_path=status_path,
                        log_path=log_path,
                        started_at_epoch=started_monotonic,
                        started_at_utc=started_wall_utc,
                        current_run_index=attempt_index,
                        runs_total=runs_total,
                        runs_completed=attempt_index - 1,
                        next_audit_epoch=time.time() + poll_seconds,
                        first_audit_epoch=first_audit_epoch,
                        proc=proc,
                        log_info=log_info,
                        exit_code=None,
                    ),
                )
                next_audit_epoch = time.time() + poll_seconds

    elapsed_seconds = time.monotonic() - started_monotonic
    finished_utc = utc_now_iso()
    log_info = inspect_log(log_path)
    exit_code = proc.returncode

    failure_reason = None
    state = "completed" if exit_code == 0 else "failed"
    if state == "failed":
        failure_reason = f"command exited with status {exit_code}"

    write_status(
        status_path,
        build_status_payload(
            run_id=run_id,
            state=state,
            command=command,
            label=label,
            git_rev=git_rev,
            baseline_payload=baseline_payload,
            status_path=status_path,
            log_path=log_path,
            started_at_epoch=started_monotonic,
            started_at_utc=started_wall_utc,
            current_run_index=attempt_index,
            runs_total=runs_total,
            runs_completed=attempt_index,
            next_audit_epoch=None,
            first_audit_epoch=first_audit_epoch,
            proc=proc,
            log_info=log_info,
            exit_code=exit_code,
            finished_at_utc=finished_utc,
            failure_reason=failure_reason,
        ),
    )

    return {
        "elapsed_seconds": elapsed_seconds,
        "pass_count": int(log_info["pass_count"]),
        "fail_count": int(log_info["fail_count"]),
        "skip_count": int(log_info["skip_count"]),
        "exit_code": exit_code,
        "log_path": log_path,
    }


def main() -> int:
    args = parse_args()
    if args.runs <= 0:
        print("ERROR: --runs must be greater than 0", file=sys.stderr)
        return 2
    if args.poll_seconds <= 0:
        print("ERROR: --poll-seconds must be greater than 0", file=sys.stderr)
        return 2

    command = list(args.command)
    if command and command[0] == "--":
        command = command[1:]
    if not command:
        command = default_command()

    csv_path = Path(args.csv_path).expanduser().resolve()
    notes_path = Path(args.notes_path).expanduser().resolve()
    baseline_path = Path(args.baseline_path).expanduser().resolve()
    runs_root = Path(args.runs_root).expanduser().resolve()
    run_id = run_id_now()
    run_root = runs_root / run_id
    status_path = run_root / "status.json"
    run_root.mkdir(parents=True, exist_ok=True)

    baseline_payload = compute_baseline_payload(
        load_timing_rows(csv_path),
        min_first_check_seconds=args.min_first_check_seconds,
        first_check_buffer_seconds=args.first_check_buffer_seconds,
        stall_threshold_seconds=args.stall_seconds,
    )
    git_rev = git_head()

    durations: list[float] = []
    last_counts = {"pass_count": 0, "fail_count": 0, "skip_count": 0}
    final_exit = 0
    last_log_path: Path | None = None

    for attempt_index in range(1, args.runs + 1):
        result = run_monitored_attempt(
            run_id=run_id,
            attempt_index=attempt_index,
            runs_total=args.runs,
            command=command,
            label=args.label,
            git_rev=git_rev,
            baseline_payload=baseline_payload,
            run_root=run_root,
            status_path=status_path,
            poll_seconds=args.poll_seconds,
            stall_seconds=args.stall_seconds,
        )

        final_exit = int(result["exit_code"])
        last_log_path = Path(result["log_path"])
        if final_exit != 0:
            break

        durations.append(float(result["elapsed_seconds"]))
        last_counts = {
            "pass_count": int(result["pass_count"]),
            "fail_count": int(result["fail_count"]),
            "skip_count": int(result["skip_count"]),
        }

    if final_exit == 0 and durations and not args.no_record_history:
        median, mean, min_seconds, max_seconds = duration_stats(durations)
        total_count = last_counts["pass_count"] + last_counts["fail_count"] + last_counts["skip_count"]
        avg_seconds_per_test = (median / total_count) if total_count else 0.0
        row = {
            "timestamp_utc": utc_now_iso(),
            "git_head": git_rev,
            "tag": args.tag,
            "runs": args.runs,
            "median_seconds": f"{median:.6f}",
            "mean_seconds": f"{mean:.6f}",
            "min_seconds": f"{min_seconds:.6f}",
            "max_seconds": f"{max_seconds:.6f}",
            "pass_count": last_counts["pass_count"],
            "fail_count": last_counts["fail_count"],
            "skip_count": last_counts["skip_count"],
            "total_count": total_count,
            "avg_seconds_per_test": f"{avg_seconds_per_test:.6f}",
            "last_run_status": "ok",
        }
        append_timing_row(csv_path, row)
        refresh_baseline_file(
            csv_path,
            baseline_path,
            min_first_check_seconds=args.min_first_check_seconds,
            first_check_buffer_seconds=args.first_check_buffer_seconds,
            stall_threshold_seconds=args.stall_seconds,
        )

        note_clean = clean_note(args.note)
        if note_clean:
            append_note(
                notes_path,
                (
                    f"- {row['timestamp_utc']} | head={git_rev} | tag={args.tag} | "
                    f"median={row['median_seconds']}s | total={total_count} | "
                    f"avg_per_test={row['avg_seconds_per_test']}s | status=ok | note={note_clean}\n"
                ),
            )

        print(f"Appended timing snapshot to: {csv_path}")
        print(f"Baseline JSON refreshed: {baseline_path}")
        print(f"run_id={run_id}")
        print(f"runs={args.runs}")
        print(f"median_seconds={median:.6f}")
        print(f"mean_seconds={mean:.6f}")
        print(
            f"pass_count={last_counts['pass_count']} fail_count={last_counts['fail_count']} "
            f"skip_count={last_counts['skip_count']} total_count={total_count}"
        )
        print(f"avg_seconds_per_test={avg_seconds_per_test:.6f}")
    else:
        baseline_payload["csv_path"] = str(csv_path)
        write_json_atomic(baseline_path, baseline_payload)
        print(f"Timing history unchanged: {csv_path}")
        print(f"Baseline JSON refreshed from current history: {baseline_path}")
        print(f"run_id={run_id}")

    if last_log_path is not None:
        print(f"latest_log_path={last_log_path}")
    print(f"status_path={status_path}")
    return final_exit


if __name__ == "__main__":
    raise SystemExit(main())
