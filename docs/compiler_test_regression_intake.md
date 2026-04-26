# Compiler Test Regression Intake

This is the public workflow for turning a failure found in a real program or
high-level canary run into permanent compiler regression coverage.

Use this document when:

- a real project exposes a compiler bug
- a canary stage fails and the problem should not stay trapped only in
  `tests/real_projects/`
- a probe is ready to be promoted into a stable suite
- a contributor needs to decide whether a failure belongs in `tests/final/`,
  `tests/binary/`, `tests/final/probes/`, or should remain only as a canary

For the broader test-system map, read `docs/compiler_test_architecture.md`.
For shared failure naming, read `docs/compiler_test_failure_taxonomy.md`.
For day-to-day command selection, read `docs/compiler_test_confidence_tiers.md`.

## Goal

The regression-intake path should be cheap enough that real failures become
permanent coverage instead of one-off debugging notes.

The intended result is:

1. the original failure is preserved
2. a focused minimized repro is added in the correct stable lane
3. the failure is classified consistently
4. optional canary coverage is retained only when it adds higher-level value

## Canonical Intake Sequence

Use this sequence:

1. Reproduce the failure in the source lane.
2. Classify the observed failure with the shared taxonomy.
3. Minimize the failure into the smallest stable repro that still proves the bug.
4. Choose the owning stable lane and bucket.
5. Decide whether the repro is ready for stable promotion or must stay probe-only first.
6. Record source metadata and provenance.
7. Re-run the narrow stable lane, then the higher-level canary if it still matters.

This is the working shorthand:

```text
discover -> classify -> reduce -> assign -> tag -> promote -> revalidate
```

## Step 1: Reproduce The Original Failure

Start from the lane where the bug was found:

- `tests/real_projects/` Stage `A` through `F`
- `tests/final/probes/`
- `tests/binary/`
- ad hoc real-program compilation outside the harnesses

Capture enough evidence to rerun it exactly:

- project or fixture name
- stage or command used
- failing target, source file, or test id
- expected behavior vs actual behavior

If the failure cannot be reproduced reliably, do not promote it yet. Keep it in
probe or canary form until the reproducer is stable enough to trust.

## Step 2: Classify The Failure

Before reducing the case, name the failure using the canonical taxonomy from
`docs/compiler_test_failure_taxonomy.md`.

Record at least:

- `failure_kind`
- `severity`
- `origin`
- `source_lane`
- `owner_lane` if already clear

Examples:

```text
failure_kind=link_fail severity=high origin=regression source_lane=real_project owner_lane=10-scopes-linkage
failure_kind=runtime_mismatch severity=critical origin=regression source_lane=probe owner_lane=14-runtime-surface
```

Do this early. It keeps the later routing and promotion steps honest.

## Step 3: Minimize The Failure

Reduce the original failure to the smallest reproducible case that still proves
the same bug.

Preferred reduction order:

1. isolate one translation unit if possible
2. remove unrelated headers, macros, and helpers
3. reduce to one behavior cluster
4. preserve the same technical failure kind
5. keep the original integration canary only if it still adds value

A good minimized repro should make it obvious:

- what compiler behavior is under test
- what oracle proves the failure
- which bucket or lane owns it

If reduction changes the technical failure, keep both:

- the original high-level canary
- the reduced stable repro

## Step 4: Choose The Owning Lane

Route the minimized repro to the narrowest stable lane that can own it.

| Situation | Preferred Lane |
|---|---|
| parser, syntax, or preprocessing-local behavior already covered by a focused legacy lane | targeted lane under `tests/parser/`, `tests/syntax/`, `tests/preprocessor/`, or `tests/codegen/` |
| canonical compiler behavior that maps to one numbered bucket | `tests/final/` |
| compile/link/runtime artifact handling or differential runtime truth | `tests/binary/` |
| still unstable, blocked, or not yet ready for a durable oracle | `tests/final/probes/` |
| higher-level practical readiness signal after a stable repro exists | keep `tests/real_projects/` canary too |

Bucket ownership rules:

- front-end and semantic language rules normally belong in buckets `01` through `12`
- IR/lowering shape belongs in bucket `13`
- runtime truth and ABI-sensitive behavior belongs in bucket `14`
- high-stress or differential edge-pressure behavior belongs in bucket `15`

If multiple buckets could plausibly own the bug, choose the narrowest primary
owner and mention the secondary surface in the PR or maintainer notes.

## Step 5: Decide Probe First vs Stable Promotion

Not every minimized repro should go directly into a stable manifest.

Keep it in `tests/final/probes/` first when:

- the oracle is still ambiguous
- the behavior is blocked by an unresolved compiler bug
- the reduction is still changing during investigation
- the case currently relies on a looser threshold or exploratory expectation

Promote it into `tests/final/` or `tests/binary/` when:

- the oracle is deterministic
- pass/fail meaning is stable
- the owning bucket or runtime lane is clear
- the result is suitable for routine confidence runs

The stable suite should stay trusted, but difficult repros should not be lost.

## Step 6: Record Source Metadata

Every promoted regression should retain its discovery provenance.

Until every harness has first-class metadata fields for this, record the intake
contract in the nearest stable place available:

- test manifest notes
- fixture comments when concise and justified
- PR description
- blocker ledger or maintainer notes for in-flight work

Minimum intake fields:

| Field | Meaning |
|---|---|
| `source_lane` | where the failure was first observed |
| `source_project` | project or fixture family that exposed it |
| `source_stage` | real-project stage or probe family when relevant |
| `origin` | `regression`, `new_gap`, `known_blocker`, or `unsupported_baseline` |
| `failure_kind` | canonical technical failure class |
| `severity` | operational importance |
| `owner_lane` | stable destination lane or bucket |
| `discovery_command` | command that reproduced the original failure |

If the failure came from `tests/real_projects/`, also record whether the canary
is being kept as:

- `retained_canary`
- `retained_until_promotion`
- `no_longer_needed`

## Step 7: Revalidate After Promotion

After adding the minimized regression:

1. run the narrow stable test or bucket
2. run the broader lane that should now own the bug
3. rerun the original canary stage when it still adds integration value

Use `docs/compiler_test_confidence_tiers.md` to choose the correct stopping
tier for that revalidation pass.

Minimum expectation:

- the new stable repro proves the bug or the fix deterministically
- the original canary no longer acts as the only place where the bug is visible

## Real-Project Failure Pattern

The common real-project path should look like this:

1. Stage `A` through `E` exposes a blocker.
2. The blocker is classified using the shared taxonomy.
3. A reduced repro is extracted into `tests/final/probes/` if still unstable.
4. Once stable, it is promoted into `tests/final/` or `tests/binary/`.
5. The real-project case remains only if it continues to prove whole-program trust.

This prevents real-project validation from turning into a graveyard of bugs that
never become routine regression coverage.

## PR And Review Expectations

When a PR fixes a regression found through this intake path, include:

- the original failing source lane
- the minimized permanent repro
- the failure classification
- the owner lane and bucket
- the exact commands rerun
- whether the higher-level canary was kept

## Related References

- `docs/compiler_test_architecture.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/compiler_test_failure_taxonomy.md`
- `docs/compiler_test_confidence_tiers.md`
- `tests/real_projects/README.md`
- `tests/final/probes/README.md`
