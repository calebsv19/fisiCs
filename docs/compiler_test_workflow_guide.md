# Compiler Test Workflow Guide (Public)

This is the practical workflow for contributors and agents adding or fixing compiler tests in `fisiCs`.

## Read Order

1. `README.md`
2. `docs/contributor_agent_quickstart.md`
3. `docs/compiler_test_system_rearchitecture_context.md`
4. `docs/compiler_test_coverage_blueprint.md`

## Standard Work Sequence

1. Pick one behavior area.
2. Confirm current status in the coverage map.
3. Add or update a minimal test case.
4. Run narrow checks first, then broader suite checks.
5. Update docs if behavior status changes.

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
```

Use narrower targets while debugging, then re-run broader checks before finalizing.

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
- doc updates when coverage/status meaningfully changed

## Public vs Private Workflow Notes

This file is intentionally public and stable.

Detailed maintainer planning and wave-by-wave internal execution logs remain private.
