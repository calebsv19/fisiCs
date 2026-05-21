# Make Final Timing Lane

This document defines the monitored `make final` timing capture system.
The goal is to track full-suite runtime as validation coverage grows.
Last updated: 2026-05-12.

## Scope

- Measure end-to-end wall time for broad `make final` checkpoint runs from the repository root.
- Record current suite size from harness output (`PASS` / `FAIL` / `SKIP` counts).
- Track average seconds per test (`median_seconds / total_count`) for trend comparisons.
- Keep detailed time-series data in maintainer audit storage configured by timing make variables.
- Keep a derived JSON baseline for first-audit prediction and live run status.
- Keep this lane separate from exact single-TU optimization acceptance work.

## Capture Command

Recommended entrypoint from repo root:

```bash
make final-monitored
```

Backward-compatible alias:

```bash
make final-timing
```

Recommended for lower-noise checkpoints:

```bash
make final-monitored FINAL_TIMING_RUNS=3 FINAL_TIMING_TAG=checkpoint
```

Optional note attachment for notable changes:

```bash
make final-monitored FINAL_TIMING_TAG=checkpoint FINAL_TIMING_NOTE="after bucket 15 manifest expansion"
```

During a live run, the monitored runner writes:

- a per-run `status.json` under `fisiCs/build/make_final_runs/<run_id>/`
- the detached broad-run stdout log beside that status file
- a derived baseline JSON under `data/fisics_timing/`

The timing system writes canonical history using make variables:

- `FINAL_TIMING_LOG` (CSV append target)
- `FINAL_TIMING_NOTES` (optional notes target when `FINAL_TIMING_NOTE` or `--note` is provided)
- `FINAL_TIMING_BASELINE` (derived baseline JSON for predicted first-audit timing)
- `FINAL_MONITORED_RUNS_ROOT` (live status/log root for monitored broad runs)

Defaults point to maintainer/internal audit locations and can be overridden per run.

Within the public trust ladder, this is the `Tier 7` maintainer checkpoint from
`docs/compiler_test_confidence_tiers.md`, not the normal inner-loop validation path.

## Relationship To Optimization Oracles

`make final-monitored` / `make final-timing` is the macro-trend lane.

For optimization work:

- use `tests/real_projects/runners/run_project_exact_compile_oracle.py` as the
  keep / drop lane
- use `tests/real_projects/runners/run_project_profile_oracle.py` as the
  attribution lane

Do not use `make final-timing` alone to accept or reject a narrow compiler
optimization.

## SQLite Mirror And Rollups

The CSV remains canonical. For query-friendly rollups, mirror CSV rows into SQLite and render markdown summaries:

```bash
make final-timing-sync
```

This command runs:

1. `make final-monitored` (monitored run + append CSV on success)
2. `make final-timing-sync-db` (CSV -> SQLite)
3. `make final-timing-rollup` (SQLite -> markdown summary)

Mirror and rollup outputs are controlled by:

- `FINAL_TIMING_DB` (SQLite mirror path)
- `FINAL_TIMING_ROLLUP` (markdown rollup output path)

Defaults are internal maintainer paths and can be overridden when needed.

You can run mirror and rollup independently:

```bash
make final-timing-refresh-baseline
make final-timing-sync-db
make final-timing-rollup
```

## CSV Columns

- `timestamp_utc`: capture timestamp (UTC)
- `git_head`: short commit hash used for the run(s)
- `tag`: freeform capture label (`manual`, `checkpoint`, `pre-release`, etc.)
- `runs`: number of full-suite executions in the capture
- `median_seconds`: median full-suite runtime across `runs`
- `mean_seconds`: arithmetic mean runtime across `runs`
- `min_seconds` / `max_seconds`: range across `runs`
- `pass_count` / `fail_count` / `skip_count`: counts from the last run in the capture
- `total_count`: `pass + fail + skip`
- `avg_seconds_per_test`: `median_seconds / total_count`
- `last_run_status`: `ok` or `fail`

## Policy

- Use `make final-monitored` for broad checkpoint runs that are expensive enough to justify file-backed supervision.
- Run captures on a stable tree (no in-flight benchmarking experiments).
- Prefer `FINAL_TIMING_RUNS=3` (or `--runs 3`) when reporting publicly.
- Treat this lane as a trend reference, not a micro-benchmark.
- On each live run, first audit time is derived from recent successful timing history with a `+30s` buffer and a `420s` floor.
- After the first audit window, status updates advance on a one-minute cadence.
- `stalled` is observational only and is driven by abnormal no-output windows; it does not kill the live run.
- Treat recent snapshot values as historical trend points, not per-change
  acceptance evidence.
- Clang comparisons are optional and can be added later as a separate column set once standardized.
