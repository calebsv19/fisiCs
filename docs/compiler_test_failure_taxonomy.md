# Compiler Test Failure Taxonomy

This is the authoritative public vocabulary for describing `fisiCs` test
failures across `tests/final/`, `tests/binary/`, `tests/final/probes/`, and
`tests/real_projects/`.

Use this document when:

- reporting a failure in a PR or issue
- deciding which maintainer lane owns a failure
- promoting a probe or real-project blocker into stable regression coverage
- aligning harness output and JSON reports onto one shared vocabulary

For the overall suite map, read `docs/compiler_test_architecture.md`.
For the permanent-regression workflow, read
`docs/compiler_test_regression_intake.md`.

## Why This Exists

The test system already had multiple useful local vocabularies:

- `tests/binary/` distinguishes compile-stage from link-stage failures
- `tests/final/probes/` distinguishes `BLOCKED`, `RESOLVED`, and `SKIP`
- `tests/real_projects/` distinguishes stage-level parity and blocker reports
- `tests/final/` distinguishes oracle mismatches by expectation type

What was missing was one shared layer above those local codes.

This taxonomy provides that shared layer.

## Canonical Report Fields

Every significant failure should eventually be representable with these fields:

| Field | Meaning |
|---|---|
| `failure_kind` | the canonical technical kind of failure |
| `severity` | how urgently the failure blocks trusted use |
| `source_lane` | where the failure was observed (`final`, `binary`, `probe`, `real_project`) |
| `trust_layer` | the confidence layer from `docs/compiler_test_architecture.md` |
| `owner_lane` | the most likely lane for the minimized permanent regression |
| `origin` | whether the failure is a new gap, a known blocker, or a regression |
| `raw_status` | the harness-native status string or code |
| `raw_detail` | the harness-native detail needed for debugging |

Not every harness emits all of these yet. This document defines the target
contract so the rollout can converge deliberately.

## Canonical Failure Kinds

These are the shared `failure_kind` values.

| `failure_kind` | Meaning | Typical Examples |
|---|---|---|
| `parser_fail` | front-end structure or recovery failure while parsing valid or intentionally malformed input | parse crash, wrong AST shape, parser recovery failure |
| `semantic_reject` | valid code is rejected or semantic analysis diverges before codegen | bad type-check rejection, scope/linkage rule rejection, invalid semantic diagnostic on valid code |
| `ir_or_codegen_fail` | lowering or code generation fails before a successful linkable artifact is produced | IR mismatch, backend crash, compiler timeout during codegen, codegen-only compile failure |
| `link_fail` | object production succeeds but the link stage fails or link-only parity diverges | unresolved symbol, duplicate-definition link error, wrong multi-TU linkage behavior |
| `runtime_crash` | produced program builds but crashes, hangs, or exits due to runtime failure | segfault, abort, timeout during runtime smoke/golden execution |
| `runtime_mismatch` | produced program runs but behavior does not match the expected runtime truth | wrong stdout, wrong stderr, wrong exit, wrong output hash, wrong artifact contents |
| `wrong_diagnostics` | compiler rejects or accepts in the expected phase, but emitted diagnostics are wrong | wrong line/column, wrong text fragment, wrong structured diagnostic payload |
| `unsupported_feature` | behavior is currently outside supported compiler capability and is intentionally tracked as not yet implemented | known unsupported C surface kept as probe or canary blocker |
| `harness_error` | the test system failed to run or validate correctly, so compiler trust was not actually measured | zero-match selector, missing expected artifact, bad manifest, missing fixture dependency |

## Severity Levels

Severity describes operational impact, not personal preference.

| `severity` | Meaning | Default Use |
|---|---|---|
| `critical` | compiler behavior is actively unsafe for trusted use | wrong-code, silent runtime mismatch, runtime crash on supported code |
| `high` | compiler rejects or fails valid program flow in a way that blocks practical adoption | semantic rejection of valid code, link failure on valid program, canary-stage parity blocker |
| `medium` | trust is weakened but the issue is narrower, more diagnosable, or less adoption-blocking | wrong diagnostics, isolated unsupported feature, harness issue blocking a targeted lane |
| `low` | reporting quality or non-gating fidelity issue with limited effect on compiler trust | telemetry-only warning drift, message polish, non-blocking report formatting issue |

## Origin Values

`origin` is separate from `failure_kind`.

This is intentional: a regression is not a technical failure mode by itself. A
regression is provenance attached to one of the canonical failure kinds above.

Recommended `origin` values:

| `origin` | Meaning |
|---|---|
| `new_gap` | first observation or not yet proven to be previously working |
| `regression` | previously working behavior that now fails |
| `known_blocker` | already tracked blocker not yet fixed |
| `unsupported_baseline` | intentionally unsupported behavior kept for visibility |

## Default Severity Rules

Use these defaults unless a lane has a stronger local reason to escalate or
de-escalate.

| Situation | Default Classification |
|---|---|
| valid program compiles but behaves incorrectly | `runtime_mismatch`, `critical` |
| valid program compiles then crashes or hangs | `runtime_crash`, `critical` |
| valid program is rejected before artifact production | `semantic_reject` or `ir_or_codegen_fail`, `high` |
| multi-TU or build-parity failure after object production | `link_fail`, `high` |
| diagnostic text/JSON does not match expected failure behavior | `wrong_diagnostics`, `medium` |
| known unsupported surface still blocked in probe/canary lane | `unsupported_feature`, `medium` |
| selector/config/artifact problem means the run did not measure intended coverage | `harness_error`, `medium` |
| warning-only telemetry drift with no gating mismatch | `harness_error`, `low` |

## Lane Mapping

This table defines how current harness-native results should be interpreted.

| Lane | Current Native Signal | Canonical Mapping |
|---|---|---|
| `tests/final/` | token/AST mismatch | usually `parser_fail` |
| `tests/final/` | diagnostics text or JSON mismatch | `wrong_diagnostics` |
| `tests/final/` | IR mismatch or compile failure in IR-focused lane | `ir_or_codegen_fail` |
| `tests/final/` | runtime stdout/stderr/exit mismatch | `runtime_mismatch` |
| `tests/final/` | fail-closed selector or missing required expectation artifact | `harness_error` |
| `tests/binary/` | `compile_fail` from harness classification | usually `ir_or_codegen_fail` or `semantic_reject`, depending fixture intent |
| `tests/binary/` | `link_fail` from harness classification | `link_fail` |
| `tests/binary/` | runtime timeout/crash | `runtime_crash` |
| `tests/binary/` | runtime stdout/stderr/exit/file mismatch | `runtime_mismatch` |
| `tests/final/probes/` | `BLOCKED` on supported target behavior | classify by technical blocker kind above |
| `tests/final/probes/` | `BLOCKED` on intentionally unsupported behavior | `unsupported_feature` |
| `tests/final/probes/` | zero-match filter or missing probe artifact | `harness_error` |
| `tests/real_projects/` Stage A | `fisics` compile fail with clang pass | usually `semantic_reject` or `ir_or_codegen_fail` |
| `tests/real_projects/` Stage B/C | build parity broken after compile succeeds | usually `link_fail` |
| `tests/real_projects/` Stage D | runtime smoke crash/timeout | `runtime_crash` |
| `tests/real_projects/` Stage D/E | runtime smoke/golden output mismatch | `runtime_mismatch` |
| `tests/real_projects/` Stage F | warning-only telemetry regression | `harness_error` with `low` severity unless made gating |

## Trust-Layer Mapping

Failure meaning should also stay aligned to the architecture layers.

| Trust Layer | Common Failure Kinds |
|---|---|
| Layer A: Fast Structural Checks | `parser_fail`, `wrong_diagnostics` |
| Layer B: Focused Behavioral Checks | `parser_fail`, `semantic_reject`, `ir_or_codegen_fail`, `wrong_diagnostics` |
| Layer C: Integration Checks | `semantic_reject`, `ir_or_codegen_fail`, `runtime_mismatch` |
| Layer D: Multi-TU / Linkage Checks | `link_fail`, `semantic_reject`, `ir_or_codegen_fail` |
| Layer E: Runtime Truth Checks | `runtime_crash`, `runtime_mismatch` |
| Layer F: Real Program / Canary Checks | any canonical kind, with practical adoption impact weighted more heavily |

## Reporting Guidance

When summarizing a failure in docs, PRs, or issue triage, prefer this shape:

```text
failure_kind=<kind> severity=<level> origin=<origin> source_lane=<lane> owner_lane=<lane>
```

Example:

```text
failure_kind=runtime_mismatch severity=critical origin=regression source_lane=real_project owner_lane=14-runtime-surface
```

## Rollout Status

Current state:

- `tests/binary/` now emits canonical failure labels in harness output while preserving the existing binary raw-status distinction (`compile_fail_unexpected`, `link_fail_unexpected`, `runtime_*`, and related codes)
- `tests/final/` now emits canonical failure labels in harness output for selector, artifact, oracle-mismatch, and runtime-mismatch paths
- `tests/final/probes/` now emits canonical blocker classification on `BLOCKED` results while preserving `BLOCKED` / `RESOLVED` / `SKIP`
- `tests/real_projects/` Stage `A` through `E` now emit canonical blocker classification in summary output and per-row JSON metadata while preserving existing stage parity and blocker semantics

Next implementation steps:

1. add canonical failure-kind/severity reporting to maintainer docs and blocker ledgers where still missing
2. tag real-project-derived regressions with `origin=regression` and owning stable lane
3. decide whether Stage `F_perf_telemetry` should emit low-severity canonical warnings or stay raw-only
4. keep public docs and harness output using the same vocabulary
5. keep the public regression-intake workflow aligned with actual promotion practice

## Related References

- `docs/compiler_test_architecture.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/compiler_test_regression_intake.md`
- `tests/final/00-harness.md`
- `tests/final/probes/README.md`
- `tests/real_projects/README.md`
