# Compiler Test System Overview (Public)

This is the public-facing test-system overview for `fisiCs`.

## Purpose

The `fisiCs` test system is designed to be fail-closed, reproducible, and practical for contributors.

Goals:

- Catch regressions early.
- Make expected behavior explicit.
- Keep test additions simple and reviewable.
- Support both human contributors and agentic tooling workflows.

## Current Test Surface

Primary commands:

```bash
make
make test
make final
make test-binary
```

Primary suites in repo:

- `tests/parser/`
- `tests/syntax/`
- `tests/codegen/`
- `tests/preprocessor/`
- `tests/spec/`
- `tests/final/` (metadata-driven harness)
- `tests/binary/` (runtime/binary validation lanes)

## Test Philosophy

- Default runs should not silently bless changed output.
- Unexpected behavior is treated as failure, not warning.
- Targeted fixes should come with targeted tests.
- Runtime/differential behavior is preferred over weak textual assertions when feasible.

## How To Add A New Test (Contributor Flow)

1. Pick a specific behavior to validate.
2. Add a minimal repro case under the relevant test area.
3. Add or update expected outputs only for that case.
4. Run the narrowest useful command first, then broader suite checks.
5. Include exact commands/results in your PR.

For metadata-driven additions, follow existing structure under:

- `tests/final/cases/`
- `tests/final/expect/`
- `tests/final/meta/`

## Required PR Evidence

At minimum (unless your change is docs-only):

```bash
make
make test
```

If your change touches final or binary behavior, include relevant runs such as:

```bash
make final
make test-binary
```

## Public vs Private Test Docs

This file is the public summary.

Deep internal planning, wave execution logs, and maintainer triage artifacts are intentionally kept in private maintainer docs.

## Related Public Docs

- `docs/contributor_agent_quickstart.md`
- `docs/compiler_test_coverage_blueprint.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/validation_workflow.md`
