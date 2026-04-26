# Compiler Test Workflow Guide (Public)

This is the practical workflow for contributors and agents adding or fixing compiler tests in `fisiCs`.

## Read Order

1. `README.md`
2. `docs/contributor_agent_quickstart.md`
3. `docs/compiler_test_architecture.md`
4. `docs/compiler_test_failure_taxonomy.md`
5. `docs/compiler_test_regression_intake.md`
6. `docs/compiler_test_confidence_tiers.md`
7. `docs/compiler_test_system_rearchitecture_context.md`
8. `docs/compiler_test_coverage_blueprint.md`

## Standard Work Sequence

1. Pick one behavior area.
2. Confirm current status in the coverage map.
3. Add or update a minimal test case.
4. Run narrow checks first, then broader suite checks.
5. Update docs if behavior status changes.
6. When recording a failure, use the shared terms from
   `docs/compiler_test_failure_taxonomy.md`.
7. When a real project or canary exposes a bug, route it through
   `docs/compiler_test_regression_intake.md` instead of leaving it only in a
   high-level lane.

## Regression Intake

Use the regression-intake workflow when a failure is discovered outside the
owning stable suite.

Required sequence:

1. reproduce the original failure in its source lane
2. classify it with the shared taxonomy
3. minimize it into the smallest stable repro
4. assign the owning stable lane or bucket
5. keep it probe-only until the oracle is deterministic
6. promote it into `tests/final/` or `tests/binary/` once stable
7. re-run the original canary only if it still adds integration value

The full contract lives in `docs/compiler_test_regression_intake.md`.

## Command Cadence

During iteration:

```bash
make
make test
```

When relevant to your change:

```bash
make final
make test-binary
make frontend-contract-test
```

Use narrower targets while debugging, then re-run broader checks before finalizing.

## Confidence Tier Selection

Use `docs/compiler_test_confidence_tiers.md` as the canonical command ladder.

Default rule:

1. start at the cheapest tier that proves the local change
2. widen to runtime truth when ABI or wrong-code risk exists
3. widen to canary stages when the bug was discovered in real project work
4. use `make final` and `make final-timing-sync` only for true trust checkpoints

## Test Addition Rules

- Prefer one behavior cluster per PR.
- Include positive, negative, and edge variants whenever practical.
- Keep expectations deterministic and reviewable.
- Avoid broad unrelated refactors in test/fix PRs.

## Fail-Closed Expectations

Treat these as failures:

- expected-pass case fails unexpectedly
- expected-fail case passes unexpectedly
- missing expected files/artifacts for required assertions
- selector/target mistakes that silently skip intended coverage

## Evidence Required In PRs

- exact commands run
- key pass/fail outputs
- expected vs actual behavior summary for bug fixes
- failure classification using the shared taxonomy when a blocker or regression is being reported
- doc updates when coverage/status meaningfully changed

## Public vs Private Workflow Notes

This file is intentionally public and stable.

Detailed maintainer planning and wave-by-wave internal execution logs remain private.
