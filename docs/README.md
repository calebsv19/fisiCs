# Docs Layout

This directory is the public documentation lane for `fisiCs`.
It is intentionally focused on stable user/contributor references.
Last audited: 2026-04-29.

## Repository Entry

- Root `README.md` is the canonical GitHub-facing project summary and quickstart.
- `TOP_README.md` is now a legacy compatibility pointer to root `README.md`.

## Top Level

- `00_docs_index.md`: primary table of contents for all Markdown docs in `docs/`
- `public_roadmap.md`: high-level public roadmap and current focus areas
- `contributor_agent_quickstart.md`: concise operating flow for contributors and automated tooling
- `compiler_test_system_rearchitecture_context.md`: core constraints and end-state targets for test architecture
- `compiler_test_architecture.md`: authoritative public architecture map for the compiler test system
- `compiler_test_failure_taxonomy.md`: shared failure-kind, severity, and origin vocabulary across compiler test lanes
- `compiler_test_regression_intake.md`: canonical reduce, classify, tag, promote, and revalidate workflow for real-world regressions
- `compiler_test_confidence_tiers.md`: canonical day-to-day command ladder from fast sanity through full trust and timing checkpoints
- `compiler_test_coverage_blueprint.md`: coverage map and harness structure for compiler validation
- `compiler_test_workflow_guide.md`: operator guide for bucket-by-bucket validation/fix workflows
- `validation_workflow.md`: full-project validation workflow for external program compile checks
- `frontend_api.md`: reusable frontend library API notes
- `extension_overlays.md`: public reference for opt-in overlay lanes and the current physics-units surface
- `compiler_ide_data_contract.md`: versioned compiler-to-IDE communication contract (current: `fisiCs.analysis.contract` `1.7.0`)
- `cli_release_workflow.md`: CLI release packaging/sign/notarize flow for macOS artifacts
- `make_final_timing_log.md`: public timing-baseline lane and capture policy for `make final`

Additional public testing references live outside `docs/` under `tests/final/`:
- bucket scope references (for example `tests/final/11-functions-calls.md`, `tests/final/12-diagnostics-recovery.md`)
- probe lane references (`tests/final/probes/README.md`, `tests/final/probes/run_probes.py`)

Real-project validation references live under `tests/real_projects/`:
- scaffold overview and stage contracts: `tests/real_projects/README.md`
- stage runners (`A`..`F`): `tests/real_projects/runners/`
- project manifest: `tests/real_projects/config/projects_manifest.json`

Current public test campaign context:
- higher-stress runtime and diagnostics validation expansion is active under `tests/final/`
- real-project compile and runtime-smoke validation continues under `tests/real_projects/`

## Private Docs Boundary

Internal bucket-level run logs, triage plans, external gap reports, and raw status logs are intentionally kept in private maintainer documentation outside this public docs lane.

Public docs in this directory should remain concise, stable, and GitHub-facing.
