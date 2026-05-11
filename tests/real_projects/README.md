# fisiCs Real-Project Validation Scaffold

This directory hosts the real-project validation ladder for `fisiCs`.

Current scope in this scaffold:
- Stage `A_tu_compile` (translation-unit compile validation)
- Stage `B_link_subset` (minimal link-subset parity)
- Stage `C_full_build` (full-build target parity)
- Stage `D_runtime_smoke` (headless runtime smoke parity)
- Stage `E_golden_behavior` (deterministic output hash/marker parity)
- Stage `F_perf_telemetry` (timing + commit + binary snapshot telemetry)
- validated projects: `datalab`, `workspace_sandbox`, `mem_console`, `line_drawing`, `ray_tracing`, `physics_sim`, `map_forge`, `ide` (Stages A-F closed)
- self-host compiler canary: `fisiCs` (Stage A clean; Stage B widened to utility + frontend contract subsets; two Stage C CLI lanes closed; six Stage D runtime-smoke targets closed; four Stage E golden-behavior targets closed; refreshed Stage F telemetry snapshot closed)
- active onboarding project: `daw` (Stage A running; blockers active)

## Layout

- `config/projects_manifest.json`
  - project metadata (root, include dirs, defines, stage settings)
- `runners/run_project_compile_tests.py`
  - Stage-A runner (`fisics` vs `clang`, per-file compile parity)
- `runners/run_project_link_subset_tests.py`
  - Stage-B runner (`fisics` objects linked by clang vs clang-only baseline)
- `runners/run_project_full_build_tests.py`
  - Stage-C runner (full-build target parity, `fisics` compile + clang link vs clang baseline)
- `runners/run_project_runtime_smoke_tests.py`
  - Stage-D runner (build parity + runtime smoke parity with timeout/exit checks)
- `runners/run_project_golden_behavior_tests.py`
  - Stage-E runner (build/runtime parity + strict golden stdout/stderr hash checks)
- `runners/run_project_perf_telemetry_tests.py`
  - Stage-F runner (non-gating telemetry snapshots and trend warnings)
- `reports/latest/`
  - latest JSON report per project/stage
- `reports/history/`
  - timestamped report snapshots
- `artifacts/latest/`
  - latest per-file command/stdout/stderr artifacts
- `artifacts/history/`
  - timestamped artifact snapshots

## Stage-A Usage

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_compile_tests.py --project datalab
# equivalent make target
make realproj-stage-a REAL_PROJECT=datalab

# repeat Stage-A for stability (default 5 runs)
make realproj-stage-a-repeat REAL_PROJECT=datalab

# override repeat count
make realproj-stage-a-repeat REAL_PROJECT=mem_console REAL_PROJECT_RUNS=5
```

Optional flags:

```bash
# run first 3 files only
python3 tests/real_projects/runners/run_project_compile_tests.py --project datalab --limit 3

# focus files by substring
python3 tests/real_projects/runners/run_project_compile_tests.py --project datalab --filter render/

# skip clang parity lane (fisics-only check)
python3 tests/real_projects/runners/run_project_compile_tests.py --project datalab --skip-clang
```

Fail-closed selector rule:
- if `--filter` and/or `--limit` reduces Stage A to zero selected sources, the runner exits with an error instead of reporting a green no-op pass

Exit codes:
- `0`: no Stage-A blockers (`fisics` compile failures with clang pass)
- `2`: blockers present
- `1`: runner/config error

## Full-TU Profiling Oracle

Use the profiling oracle runner when you need a trustworthy internal profile for
one real-project translation unit without hand-reconstructing the compile
command.

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_profile_oracle.py \
  --project physics_sim \
  --source src/app/menu/menu_state.c

python3 tests/real_projects/runners/run_project_profile_oracle.py \
  --project fisiCs \
  --source src/Syntax/Expr/analyze_expr.c

python3 tests/real_projects/runners/run_project_profile_oracle.py \
  --project physics_sim \
  --source src/app/menu/menu_state.c \
  --repeat 3 \
  --compare-summary tests/real_projects/artifacts/latest/physics_sim/profile_oracle/src/app/menu/menu_state.profile.summary.json
```

Behavior:
- rebuilds the exact `fisiCs` Stage-A compile command shape from the manifest
- runs that TU with compiler profiling enabled in `timing` mode by default
- repeats the profiled compile and reports the median duration plus median timed buckets
- scalar buckets stay off by default; opt into them only when you explicitly need a separate counting pass
- saves per-run artifacts as well as an aggregate summary for the TU
- automatically compares against the existing saved summary for that TU when present, or an explicit `--compare-summary`
- fails if the CSV contains only the header row or otherwise lacks timed samples
- saves replayable artifacts under:
  - `tests/real_projects/artifacts/latest/<project>/profile_oracle/`

Useful flags:
- `--repeat <N>`: number of measured runs to aggregate; default is `3`
- `--warmup-runs <N>`: unreported warmup runs before measured runs
- `--compare-summary <path>`: compare the new aggregate summary against a specific saved summary JSON
- `--profile-mode both`: enable timing plus scalar counters for a separate counting pass
- `--set-env NAME=VALUE`: run a profiled env-gated variant such as `DISABLE_CODEGEN=1`

## Timing System At A Glance

Use the real-project timing system in two distinct lanes:

1. `run_project_exact_compile_oracle.py`
   - the primary keep / drop lane
   - measures real wall-clock compile time without profiler overhead
   - interleaves `clang` control with `fisiCs`
   - can preflight a small sentinel TU before the main lane
2. `run_project_profile_oracle.py`
   - the attribution lane
   - turns profiling on so you can see where time is going
   - should explain a promising exact-lane result, not replace it

Core terms:

- `main lane`: the TU you are actively optimizing
- `clang control`: the paired control compile used to detect machine drift and normalize results
- `sentinel`: a small, fast TU used as a preflight health check for the timing session
- `summary baseline`: the last saved batch that was itself accepted as valid for comparison

Canonical flow:

1. run the exact compile oracle on one TU
2. keep `clang` control enabled
3. keep a sentinel TU enabled for serious sessions
4. only trust results marked `valid`
5. use `provisional` only to seed a new baseline
6. treat `noisy` as non-authoritative even if the raw `fisiCs` time looks better
7. only then use the profile oracle to explain where the time is going

Batch statuses:

- `valid`: the batch stayed in band and can be used for keep / drop decisions
- `provisional`: the batch is locally stable but there is no trusted prior baseline yet
- `noisy`: the batch drifted too much to trust
- `fisics_only`: control was intentionally disabled; useful for debugging, not canonical acceptance

What the exact oracle records:

- raw `fisiCs` durations and median
- raw `clang` durations and median
- per-run and median `fisiCs / clang` ratios
- per-run and median `fisiCs - clang` deltas
- acceptance status and validity flag
- sentinel results when configured
- exact command text for replay
- light machine-state metadata

Default interpretation rule:

- decide whether to keep a compiler change from the exact compile oracle
- decide why it helped from the profile oracle

## Exact Compile Oracle

Use the exact compile oracle when the acceptance lane should be real wall-clock
compile time for one TU without profiler overhead. This is the preferred keep /
drop lane for renderer-header optimization work such as the forced
`vk_renderer_sdl.h` `physics_sim` slice.

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_exact_compile_oracle.py \
  --project physics_sim \
  --source src/app/structural/structural_render.c \
  --repeat 3 \
  --warmup-runs 1 \
  --sentinel-source src/app/sim_runtime_3d_space.c \
  --extra-arg=-include \
  --extra-arg=../shared/vk_renderer/include/vk_renderer_sdl.h \
  --artifact-label forced_vk_renderer_sdl_exact
```

Behavior:
- rebuilds the exact Stage-A compile command shape for both `fisiCs` and `clang`
- interleaves control runs by default as `clang -> fisiCs` for every warmup and measured repeat
- explicitly scrubs `FISICS_PROFILE*` env vars so inherited shell profiling cannot contaminate acceptance runs
- reports raw `fisiCs` medians plus normalized control metrics:
  - `clang` median
  - `fisiCs / clang` ratio
  - `fisiCs - clang` delta
- marks each batch as `valid`, `provisional`, `noisy`, or `fisics_only`
  using spread checks and a control-band check against the previous saved
  `clang` median when available
- captures light machine-state metadata (`loadavg`, platform, power-source probe on macOS)
- can run an optional small sentinel TU first and fold its health into the acceptance result
- saves exact command text plus an aggregate timing summary
- supports labeled command-shape and env variants for controlled experiments
- saves artifacts under:
  - `tests/real_projects/artifacts/latest/<project>/exact_compile_oracle/`

Useful flags:
- `--repeat <N>`: number of measured runs; default is `3`
- `--warmup-runs <N>`: unreported warmup runs before measured runs
- `--extra-arg <arg>`: append exact compiler args such as forced includes
- `--set-env NAME=VALUE`: run an env-gated variant such as replay toggles
- `--compare-summary <path>`: compare a new exact-compile summary against an earlier saved summary
- `--skip-clang-control`: disable the interleaved control lane; useful only for ad hoc debugging, not canonical acceptance
- `--sentinel-source <path>` or `--sentinel-filter <substr>`: run a small preflight TU before the main lane
- `--sentinel-project <name>`: use a sentinel from another manifest project
- `--sentinel-repeat <N>`: number of measured paired runs for the sentinel; default is `1`
- `--sentinel-extra-arg <arg>`: append exact compiler args for the sentinel lane only
- `--sentinel-band-ratio <R>`: reject a batch as noisy when the sentinel drifts outside its saved band
- `--control-band-ratio <R>`: reject a batch as noisy when the current `clang` median falls outside `previous_clang / R .. previous_clang * R`
- `--max-fisics-spread-ratio <R>`, `--max-clang-spread-ratio <R>`, `--max-pair-ratio-spread <R>`: tune batch-noise thresholds

Method note:
- run exact-compile timing variants sequentially, not in parallel with other timing probes
- keep the paired `clang` control lane enabled for canonical acceptance
- keep the sentinel preflight enabled for serious optimization sessions
- treat `acceptance_status=noisy` as non-authoritative even if the raw `fisiCs` time looks good
- parallel timing runs on the forced renderer lane produced misleading outliers; the canonical acceptance lane assumes isolated sequential runs
- use `provisional` results only to seed a new baseline, not to claim a trusted improvement over prior work

Recommended workflow for heavy-header optimization:
1. run `run_project_exact_compile_oracle.py` as the primary acceptance lane
2. use a small sentinel TU such as `sim_runtime_3d_space.c` to preflight the session
3. require an in-band `clang` control result before trusting keep / drop conclusions
4. only after a promising exact-lane result, run `run_project_profile_oracle.py` to attribute where the change helped
5. keep a short-TU sanity rerun such as `sim_runtime_3d_space.c` to catch collateral regressions

Blocker reporting:
- Stage `A` through `E` blocker summaries now emit canonical classification with `failure_kind`, `severity`, `source_lane=real_project`, `trust_layer=Layer F`, `owner_lane`, and preserved `raw_status`
- JSON report rows now carry `blocker_classification` for blocker cases so downstream ledgers and reduction work can reuse the same vocabulary

## Regression Intake From Real Projects

Real-project failures should not remain only in Stage `A` through `F` reports.

Preferred path:

1. reproduce the blocker in the relevant stage
2. classify it with `docs/compiler_test_failure_taxonomy.md`
3. reduce it to a focused repro
4. route that repro into `tests/final/probes/` if still unstable
5. promote it into `tests/final/` or `tests/binary/` once the oracle is stable
6. keep the real-project case only if it still adds canary value

Use `docs/compiler_test_regression_intake.md` as the public contract for this flow.

## Next Stage Expansion

Stages `A`, `B`, `C`, `D`, `E`, and `F` are active in this scaffold.
For any expansion, follow the public stage ladder in order and keep detailed
blocker ledgers in maintainer documentation rather than this public scaffold.

## Stage-B Usage

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_link_subset_tests.py --project datalab
# equivalent make target
make realproj-stage-b REAL_PROJECT=datalab
```

Current datalab Stage-B targets:
- `datalab_smoke_data_subset`
- `datalab_pack_loader_subset`
- `datalab_data_combo_subset`

Optional flags:

```bash
# filter Stage-B targets by id substring
python3 tests/real_projects/runners/run_project_link_subset_tests.py --project datalab --target smoke

# run only fisics lane
python3 tests/real_projects/runners/run_project_link_subset_tests.py --project datalab --skip-clang
```

## Stage-C Usage

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_full_build_tests.py --project datalab
# equivalent make target
make realproj-stage-c REAL_PROJECT=datalab
```

Current datalab Stage-C targets:
- `datalab_full_default`

Optional flags:

```bash
# filter Stage-C targets by id substring
python3 tests/real_projects/runners/run_project_full_build_tests.py --project datalab --target full

# run only fisics lane
python3 tests/real_projects/runners/run_project_full_build_tests.py --project datalab --skip-clang
```

## Stage-D Usage

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_runtime_smoke_tests.py --project datalab
# equivalent make target
make realproj-stage-d REAL_PROJECT=datalab
```

Current datalab Stage-D targets:
- `datalab_runtime_headless_pack_smoke` (runs `--pack ...physics_v1_sample.pack --no-gui`)

Optional flags:

```bash
# filter Stage-D targets by id substring
python3 tests/real_projects/runners/run_project_runtime_smoke_tests.py --project datalab --target headless

# override runtime timeout
python3 tests/real_projects/runners/run_project_runtime_smoke_tests.py --project datalab --run-timeout-sec 12
```

## Stage-E Usage

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_golden_behavior_tests.py --project datalab
# equivalent make target
make realproj-stage-e REAL_PROJECT=datalab
```

Current datalab Stage-E targets:
- `datalab_golden_headless_pack_hash`

Optional flags:

```bash
# run Stage-E for selected target
python3 tests/real_projects/runners/run_project_golden_behavior_tests.py --project datalab --target golden

# override runtime timeout for golden execution
python3 tests/real_projects/runners/run_project_golden_behavior_tests.py --project datalab --run-timeout-sec 12
```

## Stage-F Usage

From `fisiCs/`:

```bash
python3 tests/real_projects/runners/run_project_perf_telemetry_tests.py --project datalab
# equivalent make target
make realproj-stage-f REAL_PROJECT=datalab
```

Stage-F behavior:
- reads latest Stage A-E reports
- emits telemetry snapshot with stage totals, commit metadata, warnings, and binary size
- default policy is warning-only (`hard_fail_on_regression=false`)

Optional flags:

```bash
# generate snapshot without writing telemetry artifacts/reports
python3 tests/real_projects/runners/run_project_perf_telemetry_tests.py --project datalab --dry-run
```
