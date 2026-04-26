# Contributor and Agent Quickstart

This file is the fastest entrypoint for human contributors and automated tooling.
Last updated: 2026-04-25.

## 1) Read First

1. `README.md` (project scope, maturity, build/test basics)
2. `docs/public_roadmap.md` (current priorities and direction)
3. `docs/compiler_test_architecture.md` (test-system map and where new tests belong)
4. `docs/compiler_test_failure_taxonomy.md` (how failures are classified and prioritized)
5. `docs/compiler_test_regression_intake.md` (how real-world failures become permanent regressions)
6. `docs/compiler_test_confidence_tiers.md` (which trust command ladder to use day to day)
7. `docs/compiler_test_workflow_guide.md` (test-work operating flow)

## 2) Build and Verify

```bash
make
make test
make frontend-contract-test
```

When relevant to your change:

```bash
make final
make test-binary
make ci-guardrails
```

Use `docs/compiler_test_confidence_tiers.md` to decide when a focused bucket run
is enough and when to widen to runtime, canary, full-suite, or timed
checkpoint validation.

## 3) Contribution Expectations

- Keep changes small and focused.
- Include exact test evidence in PRs.
- Update docs when behavior/interfaces/workflows change.
- Use `.github/CONTRIBUTING.md` as the merge/readiness contract.

## 4) Docs Boundary

- Public docs stay in `fisiCs/docs/`.
- Internal planning, deep run logs, and private triage stay outside this public docs lane.

## 5) Troubleshooting Orientation

- Compiler architecture and test-system constraints:
  - `docs/compiler_test_architecture.md`
  - `docs/compiler_test_system_rearchitecture_context.md`
  - `docs/compiler_test_coverage_blueprint.md`
- Frontend embedding/API integration:
  - `docs/frontend_api.md`
- Compiler/IDE contract compatibility and semver lane:
  - `docs/compiler_ide_data_contract.md`
- Full-project compile-validation flow:
  - `docs/validation_workflow.md`
