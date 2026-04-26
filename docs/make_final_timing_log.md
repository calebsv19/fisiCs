# Make Final Timing Lane

This document defines the `make final` timing capture system.
The goal is to track full-suite runtime as validation coverage grows.
Last updated: 2026-04-25.

## Scope

- Measure end-to-end wall time for `make final` from the repository root.
- Record current suite size from harness output (`PASS` / `FAIL` / `SKIP` counts).
- Track average seconds per test (`median_seconds / total_count`) for trend comparisons.
- Keep detailed time-series data in maintainer audit storage configured by timing make variables.

## Capture Command

Recommended entrypoint from repo root:

```bash
make final-timing
```

Recommended for lower-noise checkpoints:

```bash
make final-timing FINAL_TIMING_RUNS=3 FINAL_TIMING_TAG=checkpoint
```

Optional note attachment for notable changes:

```bash
make final-timing FINAL_TIMING_TAG=checkpoint FINAL_TIMING_NOTE="after bucket 15 manifest expansion"
```

The timing system writes using make variables:

- `FINAL_TIMING_LOG` (CSV append target)
- `FINAL_TIMING_NOTES` (optional notes target when `FINAL_TIMING_NOTE` or `--note` is provided)

Defaults point to maintainer/internal audit locations and can be overridden per run.

Within the public trust ladder, this is the `Tier 7` maintainer checkpoint from
`docs/compiler_test_confidence_tiers.md`, not the normal inner-loop validation path.

## SQLite Mirror And Rollups

The CSV remains canonical. For query-friendly rollups, mirror CSV rows into SQLite and render markdown summaries:

```bash
make final-timing-sync
```

This command runs:

1. `make final-timing` (capture + append CSV)
2. `make final-timing-sync-db` (CSV -> SQLite)
3. `make final-timing-rollup` (SQLite -> markdown summary)

Mirror and rollup outputs are controlled by:

- `FINAL_TIMING_DB` (SQLite mirror path)
- `FINAL_TIMING_ROLLUP` (markdown rollup output path)

Defaults are internal maintainer paths and can be overridden when needed.

You can run mirror and rollup independently:

```bash
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

- Run captures on a stable tree (no in-flight benchmarking experiments).
- Prefer `FINAL_TIMING_RUNS=3` (or `--runs 3`) when reporting publicly.
- Treat this lane as a trend reference, not a micro-benchmark.
- Clang comparisons are optional and can be added later as a separate column set once standardized.
