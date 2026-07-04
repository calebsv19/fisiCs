# Contributor and Agent Quickstart

This file is the fastest entrypoint for human contributors and automated tooling.
Last updated: 2026-07-03.

## 1) Read First

1. `README.md` (project scope, maturity, build/test basics)
2. `AGENTS.md` (agent boot contract and safe command boundaries)
3. `docs/first_user_path.md` (shortest supported build/example path)
4. `docs/release_confidence_checklist.md` (public release-readiness ladder)
5. `docs/compiler_test_confidence_tiers.md` (which trust command ladder to use day to day)
6. `docs/compiler_test_workflow_guide.md` (test-work operating flow)
7. `docs/public_roadmap.md` (current priorities and direction)

## 2) Build and Verify

```bash
make
make test
make frontend-contract-test
```

When relevant to your change:

```bash
make final-monitored
make test-binary
make ci-guardrails
```

Use `docs/compiler_test_confidence_tiers.md` to decide when a focused bucket run
is enough and when to widen to runtime, canary, full-suite, or timed
checkpoint validation.

## 3) Release-Readiness Agent Path

Use this path when the task is to check whether the public compiler surface is
coherent enough for a release candidate.

```bash
make
make release-contract
make examples
make examples-canaries
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
make release-archive
make release-verify
```

Then choose one changed-area final or canary gate from
`docs/release_confidence_checklist.md` if the release-readiness work touched a
specific compiler surface.

Do not edit `VERSION`, publish artifacts, update website metadata, deploy,
mutate production-registry records, or run maintainer-only remote flows unless
that scope is explicitly requested.

## 4) Contribution Expectations

- Keep changes small and focused.
- Include exact test evidence in PRs.
- Update docs when behavior/interfaces/workflows change.
- Use `.github/CONTRIBUTING.md` as the merge/readiness contract.

## 5) Docs Boundary

- Public docs stay in `fisiCs/docs/`.
- Internal planning, deep run logs, and private triage stay outside this public docs lane.

## 6) Troubleshooting Orientation

- Compiler architecture and test-system constraints:
  - `docs/compiler_test_architecture.md`
  - `docs/compiler_test_failure_taxonomy.md`
  - `docs/compiler_test_regression_intake.md`
  - `docs/compiler_test_system_rearchitecture_context.md`
  - `docs/compiler_test_coverage_blueprint.md`
- Frontend embedding/API integration:
  - `docs/frontend_api.md`
- Compiler/IDE contract compatibility and semver lane:
  - `docs/compiler_ide_data_contract.md`
- Full-project compile-validation flow:
  - `docs/validation_workflow.md`
