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
