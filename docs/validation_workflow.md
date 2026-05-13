# Full-Program Compilation Validation Workflow

Purpose: validate `fisiCs` against real projects using the staged
`tests/real_projects/` ladder, then use the exact/profile oracles correctly
when the work shifts from functional parity to compiler-efficiency tuning.
Last updated: 2026-05-12.

## Validation Ladder

The real-project scaffold is staged:

- Stage `A_tu_compile`: translation-unit compile parity
- Stage `B_link_subset`: focused link-subset parity
- Stage `C_full_build`: full-build target parity
- Stage `D_runtime_smoke`: headless runtime smoke parity
- Stage `E_golden_behavior`: deterministic output/hash parity
- Stage `F_perf_telemetry`: non-gating timing and environment telemetry

Use `tests/real_projects/README.md` as the authoritative public runner map.

## Standard Functional Workflow

1. Reproduce the issue in the narrowest real-project stage that still shows it.
2. Capture the failing TU/target and classify the blocker.
3. Reduce and fix in `fisiCs`, not in the external project source.
4. Re-run the same stage to confirm the fix.
5. Promote stable coverage into `tests/final/` or `tests/binary/` when the
   issue is no longer just a canary-only failure.
6. Re-run the source canary stage after promotion.

## When Timing Work Starts

Functional real-project stages and performance oracles are separate lanes.

### Use The Exact Compile Oracle For Acceptance

Use `tests/real_projects/runners/run_project_exact_compile_oracle.py` when the
question is whether a compiler optimization should be kept.

Rule:

- this is the keep / drop lane
- keep `clang` control enabled
- keep sentinel preflight enabled for serious sessions

### Use The Profile Oracle For Attribution

Use `tests/real_projects/runners/run_project_profile_oracle.py` after the exact
lane already shows a credible move.

Rule:

- this explains where time moved
- this does not replace the exact acceptance lane

### Use `make final-timing` Only For Macro Trend

`make final-timing` is for long-horizon suite trend tracking.

Rule:

- do not accept or reject narrow compiler optimizations from this lane

## Notes

- Keep fixes scoped to the active blocker cluster.
- Prefer fixes that unblock many files at once.
- If a failure depends on unavailable third-party headers or runtime assets,
  document the blocker clearly and skip rather than forcing local edits into the
  external project.
- Keep deep triage and execution logs in the private maintainer docs lane, not
  in this public guide.
