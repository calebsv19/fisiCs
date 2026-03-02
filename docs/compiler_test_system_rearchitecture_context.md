# Compiler Test System Re-Architecture Context

This document captures the working goal for the compiler test-system overhaul so it can be referenced during the next implementation steps.

## Scope

The target is to upgrade the compiler's current test infrastructure from an ad hoc C99-era setup into a structured, deterministic, CI-friendly system suitable for a non-trivial compiler moving toward C17.

This is not simply "add more tests." The goal is to build a test framework that reduces false confidence, prevents silent regressions, and resists oracle poisoning.

## Immediate Rule

Before changing the test system, inspect the current repository state and adapt to what exists. Do not assume layout, harness behavior, or golden-file workflows.

Inspect at minimum:

- Current `makefile` targets and test scripts
- Existing test directories and naming conventions
- How tests are invoked today
- How expected outputs and golden files are stored or updated
- Whether runtime tests exist and how they compare results
- Whether differential checks against `clang` or `gcc` exist
- How failures are surfaced (exit codes, logs, diffs)
- Any CI integration already present

## Required End State

The re-architecture should deliver:

- A single entrypoint: `make test`
- Layered targets such as `make test-lexer`, `make test-parser`, `make test-runtime`, etc.
- Test auto-discovery based on filesystem layout and/or metadata
- Deterministic, reproducible execution
- Non-destructive default runs
- Targeted golden regeneration only
- Differential runtime testing against `clang` when available
- Clear pass/fail/skip reporting with correct non-zero exits on failure

## Truth Hierarchy

Truth sources must be ranked in this order:

1. External reference compiler behavior (`clang`, when available)
2. Explicit expected runtime output and exit code
3. Explicit expected diagnostics category plus line number for negative tests
4. Stored normalized golden dumps (AST/IR/symbols)

The project compiler must never be treated as an authority for automatically regenerating its own expected outputs.

## Golden Safety Requirements

Default `make test` behavior must:

- Never modify expected or golden files
- Write generated artifacts into `build/tests/` or an equivalent isolated output tree
- Show diffs on mismatches
- Fail closed on mismatches

Regeneration must be explicit, targeted, and reviewable:

- No default "regen everything" path
- Require both explicit selection and explicit confirmation
- Supported interface should look like `make regen TEST=<id> CONFIRM=YES` or `make regen FILE=<path> CONFIRM=YES`
- Missing `TEST`/`FILE` or missing `CONFIRM=YES` must abort
- Regen should only touch selected test expectations
- Prefer temp outputs before overwrite
- Print changed files and diff summary

Recommended additions:

- `make regen-preview` to produce candidate outputs without overwriting
- An accept/review workflow that keeps changes visible in version control

When a category supports differential validation and `clang` is available, regeneration should be gated by external comparison. Divergence from reference behavior should abort regen and flag the test for review.

## Minimum Test Categories

The test system should support at least these categories:

1. `compile_ok`
   Expect compile success; may compare normalized dumps and verify emitted artifacts.
2. `compile_fail`
   Expect compile failure; verify diagnostic category and line number, not byte-for-byte messages.
3. `runtime`
   Compile, run, and compare `stdout`, `stderr`, and exit code.
4. `differential`
   Compile and run with both the project compiler and `clang`, then compare behavior when the test is safe for reference comparison.
5. `regression`
   Tie each test to a known issue/bug and never auto-modify it.

These categories should be derived from directory structure and/or per-test metadata, not hardcoded lists in `makefile`.

## Metadata Expectations

Each test should declare intent through either a small metadata file or a structured source header.

Minimum metadata fields:

- `id`
- `category`
- `standard`
- `expect_exit`
- `args` (optional)
- `run` (optional)
- `expected_stdout`
- `expected_stderr`
- `expected_diag`
- `ub`
- `skip_if` (optional)

Metadata design should extend existing repo conventions conservatively after inspection rather than replacing them blindly.

## Comparison Stability

When comparing AST/IR/symbol dumps, normalize only enough to remove noise:

- Whitespace-only differences
- Address-like values
- Temporary numbering when unstable
- Platform-specific path noise

Normalization must stay minimal and transparent. It must not hide real semantic changes.

## Harness and Makefile Design Principles

The harness may be shell or Python, but it must remain simple, auditable, deterministic, and verbose on failure.

The `makefile` should:

- Expose stable user-facing targets
- Auto-discover tests
- Keep generated artifacts out of source-controlled test trees
- Propagate exit codes correctly
- Print per-test pass/fail lines
- Emit an end-of-run summary with passed/failed/skipped counts

Parallel execution is optional. Correctness and diagnostic clarity take priority.

## Differential Testing Rules

- Detect `clang` at runtime
- If present, use it as an external behavioral reference where appropriate
- Prefer runtime behavior comparison over text diffing internal IR
- If absent, skip differential checks clearly and count them as skipped, not passed
- Tests marked with undefined behavior or target-specific behavior must be excluded from differential comparison

## Negative Test Matching Policy

Negative tests should validate stable semantics, not exact prose:

- Diagnostic category or code
- File and line number
- Optional stable substring or regex for critical phrasing

This keeps the suite resilient to message formatting changes while still checking correctness.

## Safe Failure Semantics

Unexpected behavior must fail closed:

- Expected failure but actual success -> fail
- Expected success but actual failure -> fail
- Missing expected files -> fail unless explicitly optional
- Regen without selection and confirmation -> abort

There should be no silent fallback path that converts an invalid result into a pass.

## Phased Implementation Plan

1. Discovery report
   Document current layout, commands, failure modes, and immediate risks.
2. Harness and target foundation
   Add auto-discovery, stable output dirs, summary reporting, and non-destructive `make test`.
3. Golden safety
   Add targeted `regen`, `regen-preview`, diff output, and gating checks.
4. Migration and expansion
   Move existing tests into structured categories and add representative new coverage.
5. Differential layer
   Add `clang`-backed runtime comparison plus UB/skip rules.

## Boundaries

Focus on test infrastructure, not general compiler refactoring. If the harness needs a minimal new compiler flag (for example, a stable dump mode), propose it explicitly and keep it narrowly scoped.

## Practical Goal

The final system should make it easy to add tests safely, hard to accidentally bless bad output, and obvious when behavior changes.
