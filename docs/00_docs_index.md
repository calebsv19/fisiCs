# Docs Index

This is the table of contents for the `docs/` tree.

Use this file as the fastest starting point when you need to remember what
documentation exists, where it lives, and which file to open first for a given
task.

## Recommended Read Order For Compiler Test Work

1. `compiler_test_system_rearchitecture_context.md`
2. `compiler_test_coverage_blueprint.md`
3. `compiler_behavior_coverage_checklist.md`
4. `compiler_behavior_coverage_run_log.md`
5. `compiler_test_workflow_guide.md`

## Top-Level Docs

### `docs/00_docs_index.md`

Primary table of contents for the entire `docs/` tree.

### `docs/README.md`

High-level docs layout summary. Explains the top-level docs and the major
subdirectories (`status/`, `plans/`, `reports/external/`).

### `docs/compiler_test_system_rearchitecture_context.md`

Non-negotiable constraints for the compiler test-system redesign: end-state
targets, truth hierarchy, golden safety rules, test categories, and phased
implementation boundaries.

### `docs/compiler_lexer_bucket_report.md`

First concrete bucket report for the compiler test upgrade. Captures the current
lexer baseline, existing passing coverage, confirmed lexer gaps, and the next
recommended test-expansion batch.

### `docs/compiler_preprocessor_bucket_report.md`

Baseline and preparation report for the preprocessor bucket. Captures the
current active preprocessor slice, what it already covers, the main missing
areas, and the next structured failure-collection workflow.

### `docs/compiler_declarations_bucket_report.md`

Baseline and gap report for the declarations/types bucket. Tracks current
Wave 1 coverage, added storage/qualifier tests, and confirmed spec-divergence
items to fix before bucket completion.

### `docs/compiler_codegen_bucket_report.md`

Wave 0/1 kickoff report for Phase 8 (`codegen-ir` and runtime cross-checks).
Tracks the current active baseline, newly added `13__probe_*` runtime
differential set, and the current codegen blocker inventory.

### `docs/compiler_parser_probe_backlog.md`

Current blocked parser/declaration/function probe inventory. Central list of
failing diagnostic and runtime-differential probes to fix in the next parser
stabilization phase.

### `docs/compiler_test_coverage_blueprint.md`

Repo-specific structural plan for the compiler test rework. Maps the current
test system to the target architecture, defines bucket layout, oracle strategy,
metadata expansion, and rollout phases.

### `docs/compiler_behavior_coverage_checklist.md`

Execution tracker for the compiler behavior validation campaign. Breaks the
language and compiler surface into per-feature checklist rows with status,
coverage type, planned tests, failures seen, and notes.

### `docs/compiler_behavior_coverage_run_log.md`

Rolling dated run log for bucket execution updates and wave-by-wave snapshots.
Separated from the checklist so ongoing logging does not bloat the core spec
document.

### `docs/compiler_test_workflow_guide.md`

Operator guide for how to use the compiler test docs together and how to run a
disciplined bucket-by-bucket validation and fix workflow.

### `docs/validation_workflow.md`

General validation process document for repo-wide validation passes, issue
tracking, and validation-state handling outside the compiler test redesign.

### `docs/frontend_api.md`

Reference notes for the reusable frontend library API and related frontend
integration details.

## `docs/plans/`

### `docs/plans/fisics_errors_triage_plan.md`

Structured triage plan for compiler error investigation and prioritization,
including how to work through captured failures and reduce the open error set.

### `docs/plans/runtime_bucket_14_execution_plan.md`

Active next-step plan for runtime bucket `14`, including wave-by-wave test
expansion, execution rules, and completion criteria.

### `docs/plans/link_stage_diagjson_enablement_plan.md`

Phased implementation plan for enabling structured diagnostics JSON output on
link-stage failures (`--emit-diags-json`), including driver integration points,
risk controls, rollout gates, and bucket-14 promotion strategy.

## `docs/status/`

### `docs/status/PASSING.md`

Current known passing validation cases or projects used as a positive status
baseline.

### `docs/status/FAILING.md`

Current known failing validation cases or projects used as an active issue and
regression visibility list.

### `docs/status/UNVALIDATED.md`

Items, projects, or surfaces that have not yet been fully validated and still
need explicit test or compile verification.

## `docs/reports/external/`

### `docs/reports/external/daw_errors.md`

Captured compiler errors and gap notes from the DAW external project.

### `docs/reports/external/ide_errors_summary.md`

Summary of compiler issues observed while validating the IDE external project.

### `docs/reports/external/map_forge_compiler_gap_report.md`

Detailed compiler gap report for the map-forge external project, including
missing behaviors and feature support gaps.

### `docs/reports/external/physics_sim_errors.md`

Captured compiler issues and validation notes from the physics simulation
project.

### `docs/reports/external/ray_tracing_errors.md`

Captured compiler issues and validation notes from the ray tracing project.

## Quick Navigation By Need

Open these docs based on what you are trying to do:

- Need the compiler test constraints: `compiler_test_system_rearchitecture_context.md`
- Need the compiler test structure plan: `compiler_test_coverage_blueprint.md`
- Need the compiler test execution tracker: `compiler_behavior_coverage_checklist.md`
- Need the rolling execution log: `compiler_behavior_coverage_run_log.md`
- Need the compiler test operating process: `compiler_test_workflow_guide.md`
- Need the general validation process: `validation_workflow.md`
- Need current pass/fail state: `status/PASSING.md`, `status/FAILING.md`, `status/UNVALIDATED.md`
- Need external-project failure reports: `reports/external/*.md`
- Need active error triage planning: `plans/fisics_errors_triage_plan.md`
- Need current runtime-bucket roadmap: `plans/runtime_bucket_14_execution_plan.md`
- Need link-stage `.diagjson` implementation roadmap: `plans/link_stage_diagjson_enablement_plan.md`
