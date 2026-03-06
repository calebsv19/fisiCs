# Docs Layout

This directory is organized so active reference material stays at the top level and lower-signal status/report artifacts live in focused subdirectories.

## Top Level

- `00_docs_index.md`: primary table of contents for all Markdown docs in `docs/`
- `compiler_lexer_bucket_report.md`: first bucket baseline report covering current lexer test results, confirmed gaps, and next test-expansion priorities
- `compiler_preprocessor_bucket_report.md`: preprocessor bucket baseline and prep report covering current anchor coverage, known gaps, and next execution steps
- `compiler_test_system_rearchitecture_context.md`: active working brief for the compiler test-system overhaul
- `compiler_test_coverage_blueprint.md`: concrete coverage map and harness setup plan for the compiler test suite
- `compiler_behavior_coverage_checklist.md`: execution tracker for validating each compiler behavior bucket and recording per-feature test status
- `compiler_test_workflow_guide.md`: operator guide for how to use the compiler test docs together and execute each bucket pass correctly
- `validation_workflow.md`: primary workflow for full-project validation passes
- `frontend_api.md`: reusable frontend library API notes

## Buckets

- `status/`: live validation state, baselines, and raw captured logs
- `plans/`: implementation and triage plans
- `reports/external/`: project-specific external validation reports and gap analyses

## Current High-Value Files

- `status/PASSING.md`
- `status/FAILING.md`
- `status/UNVALIDATED.md`
- `plans/fisics_errors_triage_plan.md`
