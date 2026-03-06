# Compiler Test Workflow Guide

This is the operator guide for the compiler test-system rework.

It explains how to use the current compiler-test documents together, what each
document is for, and the exact bucket-by-bucket execution flow to follow while
upgrading coverage.

Use this as the primary "how we work" reference before starting any compiler
test planning, implementation, validation, or bucket-level bug-fix pass.

## Core Working Set

These are the four primary documents for the compiler test effort:

1. `docs/compiler_test_system_rearchitecture_context.md`
2. `docs/compiler_test_coverage_blueprint.md`
3. `docs/compiler_behavior_coverage_checklist.md`
4. `docs/compiler_test_workflow_guide.md`

They serve different roles and should not be treated as duplicates.

## What Each Document Does

### 1. Re-Architecture Context

File:

- `docs/compiler_test_system_rearchitecture_context.md`

Purpose:

- Defines the non-negotiable constraints for the test-system redesign.
- Sets the truth hierarchy.
- Defines golden safety rules.
- Defines required end-state targets like `make test`, layered `make test-*`,
  targeted `regen`, deterministic artifacts, and safe failure semantics.

Use this when:

- Deciding whether a harness change is allowed
- Evaluating whether a proposed workflow violates golden safety
- Checking the required end-state for the new test system

This doc answers:

- "What must the system guarantee?"

### 2. Coverage Blueprint

File:

- `docs/compiler_test_coverage_blueprint.md`

Purpose:

- Maps the test re-architecture into the current repository reality.
- Explains how existing `makefile`, legacy shell tests, and `tests/final/`
  should converge.
- Defines the canonical bucket layout, oracle strategy, metadata expansion, and
  rollout phases.

Use this when:

- Planning harness implementation work
- Deciding where new tests belong
- Deciding which oracle a test should use
- Deciding how `tests/final` should evolve

This doc answers:

- "How should the system be structured here, in this repo?"

### 3. Behavior Coverage Checklist

File:

- `docs/compiler_behavior_coverage_checklist.md`

Purpose:

- Acts as the bucket-by-bucket execution tracker.
- Breaks the language and compiler behavior surface into explicit feature rows.
- Provides the fields needed to track valid, negative, and edge coverage, plus
  current failures and unsupported behavior.

Use this when:

- Running a bucket validation campaign
- Adding planned test ids
- Recording failures before fixing them
- Marking support, gaps, or unsupported features

This doc answers:

- "What exact behavior in this bucket must be tested, and what is its current status?"

### 4. Workflow Guide

File:

- `docs/compiler_test_workflow_guide.md`

Purpose:

- Defines how to move through the work without losing discipline.
- Tells us what to read first, what to update during a run, and what order to
  use when converting a spec into tests and fixes.

Use this when:

- Starting a new bucket
- Handing off work across messages
- Checking whether the next action should be discovery, test creation,
  reporting, fixing, or harness work

This doc answers:

- "What is the correct process for doing the work?"

## Document Usage Order

For any new compiler test task, use the docs in this order:

1. Read `compiler_test_system_rearchitecture_context.md`
2. Read `compiler_test_coverage_blueprint.md`
3. Read the relevant section in `compiler_behavior_coverage_checklist.md`
4. Use `compiler_test_workflow_guide.md` to execute the bucket correctly

That order matters:

- Context first, so constraints are fresh
- Structure second, so the repo-specific plan is clear
- Bucket checklist third, so the exact feature surface is in scope
- Workflow guide throughout, so execution stays disciplined

## Required Behavioral Rules

These are the standing rules for the work:

- Do not assume unsupported features are supported.
- Mark unsupported features explicitly.
- Do not auto-regenerate goldens during normal test runs.
- Do not fix a bucket after only one or two spot checks.
- First collect the full failure set for the active bucket.
- Then fix the bucket as a grouped behavior pass.
- Re-run the whole bucket after fixes.
- Prefer stronger oracles over weaker ones:
  `differential` > `runtime` > `diag` > `ast/ir/tokens`
- Use runtime behavior over textual IR comparisons where practical.
- Tag UB, implementation-defined, and ABI-sensitive cases explicitly.
- Never let a missing expectation silently downgrade into a pass.

## Bucket Execution Flow

This is the standard per-bucket operating sequence.

### Step 1: Select One Bucket

Choose one bucket only, such as:

- `lexer`
- `preprocessor`
- `declarations`
- `expressions`
- `semantic`
- `codegen-ir`
- `runtime`
- `regression`

Do not mix multiple large buckets in one validation pass unless there is a clear
reason. Focus is how the failure set stays understandable.

### Step 2: Load The Relevant Checklist Section

Open the matching section in:

- `docs/compiler_behavior_coverage_checklist.md`

For that bucket:

- Confirm the feature rows in scope
- Confirm the expected oracle types
- Confirm any special notes such as unsupported markers, UB tags, or ABI
  sensitivity

### Step 3: Map Current Tests To Features

Before writing new tests, map what already exists:

- Existing `tests/final/cases/*`
- Existing `tests/final/expect/*`
- Existing `tests/final/meta/*.json`
- Relevant legacy shell tests under `tests/parser/`, `tests/syntax/`,
  `tests/codegen/`, `tests/preprocessor/`, or `tests/spec/`

For each feature row:

- Note which test already exists
- Note which test is missing
- Note whether current coverage is only positive and still lacks negative or
  edge variants

Update the checklist `Planned Tests` field accordingly.

### Step 4: Implement Or Complete The Bucket Test Set

For each feature row, make sure the bucket eventually includes:

- A valid case
- A negative case where applicable
- An edge or stress case

When implementing:

- Put new source cases under the canonical suite (`tests/final/cases/`)
- Put checked-in expectations under `tests/final/expect/`
- Add or extend metadata in the relevant `tests/final/meta/*.json` bucket shard
- Keep `tests/final/meta/index.json` as the manifest registry
- Keep naming stable and bucket-aligned
- If the current harness cannot faithfully express the needed oracle, fix the
  harness before adding the test so the test does not bless the wrong behavior

If a case reproduces a real bug but there is no acceptable passing oracle yet:

- Add the repro source under `tests/final/cases/`
- Document it as a known issue case
- Keep it out of active metadata until it can pass with an intentional oracle

This preserves the investigation artifact without forcing the main suite to
bless broken behavior or fail continuously without an xfail policy.

Do not skip negative coverage just because the valid case already passes.

### Step 5: Run The Entire Bucket

Once the bucket has enough tests to be meaningful:

- Run the whole bucket target, not isolated single tests only
- Capture all failures before starting fixes

The purpose of this step is to produce a failure report for the bucket, not to
chase each failure immediately.

This is the point where the checklist should be updated:

- Mark rows as `in_progress`, `blocked`, `passing`, or `unsupported`
- Fill in `Failures Seen`
- Record unsupported behavior explicitly instead of leaving it blank

### Step 6: Produce The Bucket Failure Report

Before fixing code, summarize the bucket state:

- Which feature rows are passing
- Which are failing
- Which are unsupported
- Which lack negative coverage
- Which lack edge coverage
- Which failures cluster into the same root cause

This report is what makes the fix pass efficient.

### Step 7: Fix The Bucket As A Group

After the bucket failure set is visible:

- Group related failures by root cause
- Fix the implementation in coherent batches
- Avoid bouncing between unrelated buckets unless a shared compiler bug forces it

The unit of repair is the bucket behavior group, not a single isolated file.

### Step 8: Re-Run The Full Bucket

After fixes:

- Re-run the bucket target
- Re-check all rows in the checklist section
- Confirm that fixes did not break adjacent features in the same bucket

Only after the full bucket is stable should that section be considered closed.

## What To Update During A Bucket Pass

During active work, keep these artifacts aligned:

### Update The Checklist

Always update:

- `docs/compiler_behavior_coverage_checklist.md`

This is the authoritative status tracker for per-feature progress.

Update at minimum:

- `Planned Tests`
- `Status`
- `Failures Seen`
- `Notes`

### Update The Harness Inputs

As tests are added or migrated:

- `tests/final/cases/`
- `tests/final/expect/`
- `tests/final/meta/index.json`

These are the authoritative suite assets.

### Update The Blueprint Only When Structure Changes

Only update:

- `docs/compiler_test_coverage_blueprint.md`

When:

- Bucket structure changes
- Oracle policy changes
- Metadata schema changes
- Harness behavior changes
- Rollout phase assumptions change

Do not use the blueprint as a per-test status log.

### Update The Context Only When Core Constraints Change

Only update:

- `docs/compiler_test_system_rearchitecture_context.md`

When:

- A core system requirement changes
- Golden safety policy changes
- Truth hierarchy changes
- Required end-state targets change

This should be rare.

## How To Think About Oracles

Use the strongest stable oracle available for the behavior being tested.

Preferred order:

1. `differential`
2. `runtime`
3. `diag`
4. `ast`
5. `ir`
6. `tokens`

Practical guidance:

- Use `tokens` for tokenization and preprocessor stream checks.
- Use `ast` for grammar shape and parse disambiguation.
- Use `diag` for compile failures and unsupported-feature signaling.
- Use `ir` only when IR structure itself is the behavior under test.
- Use `runtime` when executable behavior can verify semantics more directly.
- Use `differential` when the test is safe, deterministic, and free of UB.

If a test can be verified by runtime behavior, do not rely only on a textual IR
golden unless there is a specific structural reason to do so.

## Unsupported, UB, And Implementation-Defined Rules

These categories must be explicit in both planning and execution.

### Unsupported

If a feature is intentionally unsupported:

- Mark the checklist row `unsupported`
- Add or preserve a diagnostic test that proves the rejection is stable
- Note the limitation in `Notes`

Unsupported is acceptable. Silent ambiguity is not.

### Undefined Behavior

If a test uses UB:

- Tag it in metadata
- Mark it in checklist notes
- Do not use strict differential runtime comparison as the primary oracle

UB tests can still be useful, but they cannot anchor correctness comparisons.

### Implementation-Defined Or ABI-Sensitive

If behavior depends on:

- signed `char`
- enum sizing
- padding
- layout
- calling convention
- float formatting

Then:

- Tag it explicitly
- Avoid brittle exact comparisons across toolchains unless the comparison is
  scoped carefully

## What A Good Bucket Report Looks Like

Each bucket report should include:

- Bucket name
- Feature scope covered
- Existing tests mapped
- New tests added
- Passing count
- Failing count
- Unsupported count
- Missing coverage count
- Main root-cause clusters
- Recommended fix batch order

This is the output we want before deeper implementation work.

## Immediate Next-Action Pattern

When starting a new pass, use this mental sequence:

1. Refresh context docs
2. Select one bucket
3. Audit existing tests for that bucket
4. Fill the checklist rows with planned tests
5. Run the bucket
6. Write the failure report
7. Fix the bucket
8. Re-run and close the bucket

That sequence is the standard operating method for this project until the new
test system is fully mature.
