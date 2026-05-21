#!/usr/bin/env python3
"""Refresh the derived make-final timing baseline JSON from canonical CSV."""

from __future__ import annotations

import argparse
from pathlib import Path

from make_final_timing_common import DEFAULT_BASELINE_PATH
from make_final_timing_common import DEFAULT_LOG_PATH
from make_final_timing_common import refresh_baseline_file


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--csv", dest="csv_path", default=str(DEFAULT_LOG_PATH), help="Canonical timing CSV path")
    parser.add_argument("--output", dest="baseline_path", default=str(DEFAULT_BASELINE_PATH), help="Baseline JSON output path")
    parser.add_argument("--min-first-check-seconds", type=int, default=420, help="Minimum first audit window in seconds")
    parser.add_argument("--first-check-buffer-seconds", type=int, default=30, help="Buffer added to the timing baseline before first audit")
    parser.add_argument("--stall-seconds", type=int, default=900, help="Observational no-output stall threshold")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    csv_path = Path(args.csv_path).expanduser().resolve()
    baseline_path = Path(args.baseline_path).expanduser().resolve()

    payload = refresh_baseline_file(
        csv_path,
        baseline_path,
        min_first_check_seconds=args.min_first_check_seconds,
        first_check_buffer_seconds=args.first_check_buffer_seconds,
        stall_threshold_seconds=args.stall_seconds,
    )

    print(f"Baseline JSON: {baseline_path}")
    print(f"sample_count={payload['sample_count']}")
    print(f"baseline_source={payload['baseline_source']}")
    print(f"baseline_seconds={payload['baseline_seconds']}")
    print(f"recommended_first_check_seconds={payload['recommended_first_check_seconds']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
