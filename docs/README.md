# Docs Layout

This directory is the public documentation lane for `fisiCs`.
It is intentionally focused on stable user/contributor references.

## Repository Entry

- Root `README.md` is the canonical GitHub-facing project summary and quickstart.
- `TOP_README.md` is now a legacy compatibility pointer to root `README.md`.

## Top Level

- `00_docs_index.md`: primary table of contents for all Markdown docs in `docs/`
- `public_roadmap.md`: high-level public roadmap and current focus areas
- `contributor_agent_quickstart.md`: concise operating flow for contributors and agentic tooling
- `compiler_test_system_rearchitecture_context.md`: core constraints and end-state targets for test architecture
- `compiler_test_coverage_blueprint.md`: coverage map and harness structure for compiler validation
- `compiler_test_workflow_guide.md`: operator guide for bucket-by-bucket validation/fix workflows
- `validation_workflow.md`: full-project validation workflow for external program compile checks
- `frontend_api.md`: reusable frontend library API notes
- `compiler_ide_data_contract.md`: versioned compiler-to-IDE communication contract (current: `fisiCs.analysis.contract` `1.4.0`)
- `cli_release_workflow.md`: CLI release packaging/sign/notarize flow for macOS artifacts

Additional public testing references live outside `docs/` under `tests/final/`:
- bucket scope references (for example `tests/final/11-functions-calls.md`, `tests/final/12-diagnostics-recovery.md`)
- probe lane references (`tests/final/probes/README.md`, `tests/final/probes/run_probes.py`)

## Private Docs Boundary

Internal bucket-level run logs, triage plans, external gap reports, and raw status logs are intentionally kept in private maintainer documentation outside this public docs lane.

Public docs in this directory should remain concise, stable, and GitHub-facing.
