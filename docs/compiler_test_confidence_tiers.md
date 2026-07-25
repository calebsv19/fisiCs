# Compiler Test Confidence Tiers

This is the public command ladder for day-to-day `fisiCs` compiler work.

Use this document to decide:

- which test command is enough for the change in front of you
- when to widen from a local bucket check to runtime or canary validation
- when a full trust checkpoint is warranted
- when timing capture belongs in the workflow instead of normal iteration

For the overall test-system map, read `docs/compiler_test_architecture.md`.
For real-world regression promotion, read
`docs/compiler_test_regression_intake.md`.

## Goal

Normal compiler work should not rely on memory of many unrelated commands.

The intended model is simple:

1. start with the cheapest tier that proves the local change
2. widen only when the change touches a broader trust surface
3. reserve full-suite and timing checkpoints for the points where they add real value

## Tier Ladder

| Tier | Name | Primary Purpose | Typical Commands | Cost | Use When |
|---|---|---|---|---|---|
| `Tier 0` | Buildable Tree | confirm the compiler still builds | `make` | lowest | any code change before deeper validation |
| `Tier 1` | Fast Sanity | catch broad front-end and legacy regressions quickly | `make test` | low | most parser, semantic, and infrastructure edits |
| `Tier 2` | Focused Owner Lane | prove the owning stable lane for the exact behavior you changed | `make final-id ID=<test_id>`, `make final-bucket BUCKET=<bucket>`, `make final-manifest MANIFEST=<manifest>.json`, `make test-binary-id ID=<test_id>` | low to medium | one bucket, one runtime case, one binary case, or one stable regression fix |
| `Tier 3` | Runtime Truth | catch wrong-code, ABI, runtime-output, and link/runtime surface regressions | `make final-runtime`, `make test-binary`, `make os-policy` when the change affects freestanding policy | medium | runtime, codegen, ABI, multi-TU, binary-surface, or OS-policy changes |
| `Tier 4` | Regression Frontier | work an unstable or newly discovered failure before stable promotion | `PROBE_FILTER=<id> python3 tests/final/probes/run_probes.py`, plus the source canary rerun | medium | new blocker reduction, probe work, or real-project-derived bug reduction |
| `Tier 5` | Canary Program | confirm practical readiness on a real project stage | run the matching `tests/real_projects/runners/run_project_*_tests.py` stage | medium to high | project-adoption work, real-world regression fixes, or canary validation after a stable repro is added |
| `Tier 6` | Full Trust Run | checkpoint the stable compiler behavior suite before handoff or larger merge points | `make final-monitored` plus any relevant `make test-binary` and/or real-project stage | high | before merge, before switching back to normal project work, or after broad compiler fixes |
| `Tier 7` | Timed Checkpoint | record performance-trend evidence for the full trust run | `make final-timing-sync` | highest | notable checkpoints, trend tracking, and maintainer audit captures rather than routine edit loops |

## Default Stopping Rules

Use these default stopping points unless the change or failure says otherwise.

| Change Type | Minimum Tier |
|---|---|
| parser, syntax, or non-runtime semantic fix | `Tier 2` |
| codegen, ABI, linkage, or runtime behavior fix | `Tier 3` |
| freestanding target or OS-policy behavior change | `Tier 3` plus the affected OS-P tier |
| probe-only blocker reduction or unstable repro work | `Tier 4` |
| real-project-derived regression fix | `Tier 5` |
| broader trust checkpoint before handoff or merge | `Tier 6` |
| timing-baseline capture or audit milestone | `Tier 7` |

## How To Pick The Right Tier

### Start Narrow

If you already know the owning stable lane, begin there:

- one stable test id
- one bucket
- one binary test id
- one real-project target only when the bug is truly canary-derived

Do not jump to `make final-monitored` first unless the change is already broad enough to
justify it.

### Widen On Surface Change

Widen one tier at a time when the change affects:

- multiple compiler buckets
- multi-TU behavior
- runtime behavior
- ABI-sensitive layouts or calling surfaces
- canary adoption confidence for a real project

### Use Probes Only For Frontier Work

`tests/final/probes/` is not the normal validation lane for everyday edits.
Use it when:

- the behavior is still blocked
- the oracle is not yet stable enough for promotion
- you are reducing a newly discovered real-world failure

Once the oracle is deterministic, move back to the owning stable lane.

### Treat Compiler Exit Status As Part Of The Oracle

Stable reject tests must assert the compiler status as well as diagnostic
text or JSON. A completed compilation with no errors exits `0`; any emitted
parser or semantic error exits `1`, including recovered parser errors and
multi-translation-unit runs. Parser recovery may continue far enough to
collect ordered diagnostics, but it must not proceed to code generation or
linking after an error.

Legacy AST goldens can opt into an exact status oracle with a sibling
`<golden>.exit` file containing `0` or `1`. Final manifests should use their
explicit compile-exit field. Do not use `|| true` or an arbitrary-nonzero
check around a compiler invocation when the fixture has a deterministic
status contract.

For multi-translation-unit diagnostic tests, validate both input orders when
order matters. Structured expectations should retain each translation unit's
rich payload, including include-stack and macro-trace details, after the
individual frontend context has been destroyed.

## Common Work Patterns

### Pattern A: Stable Bucket Fix

Use this when the bug already belongs to a canonical bucket and has a stable
oracle.

1. `make`
2. `make final-id ID=<test_id>` or `make final-bucket BUCKET=<bucket>`
3. widen to `make final` only if the change touched broader shared behavior

### Pattern B: Runtime Or ABI Fix

Use this when wrong-code or runtime behavior is at risk.

1. `make`
2. focused stable lane check (`make final-id`, `make final-bucket`, or `make test-binary-id`)
3. `make final-runtime` and/or `make test-binary`
4. `make final-monitored` if the change is broad enough to justify a full trust checkpoint

### Pattern C: Real-Project Regression

Use this when the failure was discovered in `tests/real_projects/` or normal
project work.

1. rerun the failing canary stage
2. follow `docs/compiler_test_regression_intake.md`
3. reduce into `tests/final/probes/` only if the repro is still unstable
4. assign explicit ownership in `tests/final/` or `tests/binary/` once stable;
   source files may remain under the probe tree when the stable manifest owns
   the exact reduced fixture
5. rerun the source canary stage

### Pattern D: Maintainer Checkpoint

Use this when you want a durable trust and timing checkpoint.

1. `make final-monitored`
2. relevant `make test-binary` or real-project stage if the recent work touched those surfaces
3. `make final-timing-sync`

Use this pattern for milestone captures, not normal inner-loop iteration.

### Pattern E: OS-Policy Regression

Use this when the behavior comes from freestanding kernel/user policy C.

1. run the focused OS-P case and tier
2. preserve the explicit expected result, object contract, and guest
   serial/exit/parity contract when QEMU evidence applies
3. reduce compiler-local behavior into its numbered owner bucket
4. enter fix mode only after the active blocker ledger is coherent
5. rerun `make os-policy`
6. use `make final-monitored` only when the reduced owner case is ready for a
   broad promotion checkpoint

## Cost Guidance

These labels are intentionally rough. They are for workflow choice, not for
benchmark claims.

- `lowest`: build-only confirmation
- `low`: quick broad sanity or one stable owner-lane check
- `medium`: runtime-heavy or focused frontier work
- `medium to high`: real-project canary stage
- `high`: full stable trust checkpoint
- `highest`: full trust checkpoint plus timing capture

## Relationship To Other Public Docs

- `docs/compiler_test_architecture.md` explains what each lane means
- `docs/compiler_test_failure_taxonomy.md` explains how failures are named
- `docs/compiler_test_regression_intake.md` explains how real failures become permanent regressions
- `docs/compiler_test_workflow_guide.md` explains the general contributor flow
- `docs/make_final_timing_log.md` explains the timing-capture policy behind `Tier 7`
- `docs/os_policy_validation.md` explains the OS-P authority, evidence, and
  promotion boundary

## Related References

- `docs/compiler_test_architecture.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/compiler_test_regression_intake.md`
- `docs/make_final_timing_log.md`
- `tests/real_projects/README.md`
