# Compiler Test Architecture

This is the authoritative public architecture reference for the `fisiCs` test
system.

Use this document to understand:

- what test lanes exist
- what each lane is supposed to validate
- what a pass or failure means
- where a new test should be added
- which command should be run during development, promotion, and regression work

For execution cadence and contribution flow, also read:

- `docs/compiler_test_workflow_guide.md`
- `docs/compiler_test_failure_taxonomy.md`
- `docs/compiler_test_regression_intake.md`
- `docs/compiler_test_confidence_tiers.md`
- `docs/compiler_test_coverage_blueprint.md`
- `docs/validation_workflow.md`

## Goals

The test system exists to do four things reliably:

1. Catch regressions quickly.
2. Expose failures clearly by compiler stage and runtime effect.
3. Prefer deterministic, reviewable oracles over ambiguous “looks okay” checks.
4. Support compiler adoption in real programs, not only synthetic fixtures.

## Design Principles

- Fail closed: an unexpected output, missing artifact, or wrong selector should be a failure.
- Layered confidence: small fast lanes and larger trust lanes should coexist instead of forcing one giant workflow.
- Stable ownership: each test should have a clear home by behavior and compiler stage.
- Promotion over drift: probe-only repros should be promoted into stable suites once they become reliable.
- Real-world intake: failures found in real programs should be reduced into permanent regressions.

## System Map

| Lane | Primary Purpose | Primary Oracle | Typical Use | Main Entry Points |
|---|---|---|---|---|
| `tests/parser/` | focused parser and grammar checks | shell-script pass/fail, AST/diagnostic expectations | parser-local debugging and legacy targeted checks | `make test`, parser-specific targets |
| `tests/syntax/` | semantic and language-rule validation | shell-script pass/fail and diagnostics | focused semantic debugging | `make test`, syntax-specific targets |
| `tests/codegen/` | targeted lowering/codegen behavior | shell-script compile/run checks | focused backend debugging | `make test`, codegen-specific targets |
| `tests/preprocessor/` | macro, include, conditional, and preprocessing behavior | shell-script pass/fail and text expectations | preprocessor-local debugging | `make test`, preprocessor-specific targets |
| `tests/spec/` | stable AST golden checks | AST goldens | parser and semantic structure regression checks | `make test`, `spec-tests` |
| `tests/final/` | canonical bucketed compiler behavior suite | tokens, AST, diagnostics, IR, runtime stdout/stderr/exit | main behavior validation and bucket-level promotion | `make final`, `make final-bucket`, `make final-manifest`, `make final-id` |
| `tests/binary/` | compile/link/runtime validation with runtime artifacts and differential checks | compile result, link result, stdout, stderr, exit, runtime files | runtime truth checks and binary-surface regression work | `make test-binary`, `make test-binary-id` |
| `tests/final/probes/` | high-risk repros and pre-promotion exploration | blocked/resolved probe status vs compiler and reference behavior | probe mode, blocker collection, reduced-threshold exploration | `python3 tests/final/probes/run_probes.py` |
| `tests/real_projects/` | staged canary validation against full programs | parity reports, build success, runtime smoke, golden behavior, telemetry | practical compiler adoption and real-world regression intake | stage runners `A` through `F` |

## Confidence Layers

The full architecture is designed as layered confidence rather than one flat list
of tests.

| Layer | Purpose | Primary Lanes |
|---|---|---|
| Layer A: Fast Structural Checks | catch obvious front-end and syntax regressions quickly | `make test`, `tests/spec/`, focused parser/syntax/preprocessor lanes |
| Layer B: Focused Behavioral Checks | isolate one rule or behavior deeply | `tests/final/` bucket slices, targeted shell tests |
| Layer C: Integration Checks | validate multi-feature continuity within one translation unit or small feature cluster | `make final`, selected manifests, selected binary lanes |
| Layer D: Multi-TU / Linkage Checks | validate symbol resolution, translation-unit interaction, and link-stage behavior | `tests/final/` multi-input cases, `tests/binary/`, real-project Stage B and C |
| Layer E: Runtime Truth Checks | detect wrong-code and runtime mismatches | `tests/final/` runtime cases, `tests/binary/`, real-project Stage D and E |
| Layer F: Real Program / Canary Checks | validate practical readiness on actual projects | `tests/real_projects/` Stage A through F |

## The Canonical Bucket Suite

`tests/final/` is the canonical metadata-driven behavior suite. It organizes the
compiler into numbered behavior buckets so that contributors can place tests in
the correct semantic lane.

| Bucket | Name | Primary Focus |
|---|---|---|
| `01` | translation phases | phase ordering, macro-driven parse changes, line mapping, deterministic outputs |
| `02` | lexer | tokenization, malformed literals, raw token-frontier diagnostics |
| `03` | preprocessor | macro expansion, include resolution, conditional logic, directive diagnostics |
| `04` | declarations | declarators, type specifiers, aggregate declarations, declaration parsing |
| `05` | expressions | expression parsing, precedence, operator semantics, expression diagnostics |
| `06` | lvalues-rvalues | addressability, modifiable lvalues, qualifier-loss and assignment legality |
| `07` | types-conversions | promotions, conversions, constant-expression legality, pointer and arithmetic conversions |
| `08` | initializers-layout | initializer legality, object layout, designators, aggregate layout rules |
| `09` | statements-controlflow | statement forms, loop/switch/goto behavior, control-flow legality |
| `10` | scopes-linkage | namespaces, redeclarations, linkage rules, multi-TU name conflicts |
| `11` | functions-calls | prototypes, calls, returns, variadics, function-pointer call surfaces |
| `12` | diagnostics-recovery | parser recovery, malformed input handling, diagnostic stability |
| `13` | codegen-ir | lowering semantics and IR-level behavior |
| `14` | runtime-surface | runtime execution, ABI-sensitive behavior, library/header surface, runtime regressions |
| `15` | torture-differential | high-stress, differential, and edge-pressure behavior |

The per-bucket references under `tests/final/*.md` define bucket-local scope,
acceptance criteria, and example cases.

## Oracle Types

Different lanes use different kinds of truth checks. Contributors should choose
the narrowest oracle that proves the behavior.

| Oracle | Use When |
|---|---|
| token stream | validating lexer/preprocessor token shape |
| AST text | validating parse structure and deterministic front-end shape |
| diagnostics text | validating human-facing failure behavior |
| diagnostics JSON | validating structured diagnostic location and taxonomy fields |
| IR text | validating lowering shape where IR is the most direct truth signal |
| runtime stdout/stderr/exit | validating end-to-end behavior and wrong-code detection |
| runtime file artifacts | validating file-producing binaries or side-effecting runs |
| differential comparison | validating that runtime behavior matches a trusted host compiler |
| staged canary parity | validating practical readiness on real programs |

## Shared Failure Vocabulary

The authoritative public failure vocabulary now lives in
`docs/compiler_test_failure_taxonomy.md`.

Use it to answer:

- what kind of failure occurred
- how severe that failure is for practical compiler trust
- which stable lane should ultimately own the minimized regression

This architecture doc explains where failures happen. The taxonomy doc explains
how they should be named consistently across suites.

## Stable Lanes vs Probe Lanes

The test system separates stable validation from exploratory repro collection.

| Lane Type | Meaning |
|---|---|
| Stable suite | should be green in normal use and is suitable for routine confidence checks |
| Probe lane | used to collect blockers, explore fragile surfaces, or keep hard repros alive before promotion |
| Canary lane | validates whole-program readiness at a higher integration level |

Probe fixtures under `tests/final/probes/` are intentionally not part of
`make final`. They exist to preserve hard repros, accelerate focused debug
cycles, and support promotion into stable manifests once the oracle is reliable.

## Promotion Lifecycle

The intended lifecycle for new coverage is:

1. discover a gap or regression
2. add a focused repro in the right lane
3. keep it probe-only if behavior is still unstable or blocked
4. promote it into `tests/final/` or `tests/binary/` once it has a stable oracle
5. keep a real-project canary only when the higher-level case still adds value

This keeps the stable suite clean without losing difficult repros.

For the exact workflow used to turn real failures into permanent regressions,
read `docs/compiler_test_regression_intake.md`.

## Real-Project Canary Architecture

`tests/real_projects/` is the practical readiness ladder above the synthetic
suites.

| Stage | Purpose |
|---|---|
| `A_tu_compile` | compile translation units individually and compare parity |
| `B_link_subset` | validate minimal linkable target subsets |
| `C_full_build` | validate full project build parity |
| `D_runtime_smoke` | validate deterministic runtime smoke behavior |
| `E_golden_behavior` | validate deterministic output-hash and marker behavior |
| `F_perf_telemetry` | record non-gating telemetry and trend warnings |

Real-project failures should not stay trapped only in this lane. When a real
program exposes a compiler bug, the preferred path is:

1. identify the failure in the real project
2. minimize the failure into a focused repro
3. place the minimized test into the owning bucket
4. keep the canary if it still adds integration value

The full reduce, classify, tag, promote, and revalidate contract is documented
in `docs/compiler_test_regression_intake.md`.

## Choosing Where A New Test Belongs

Use this routing rule:

1. If the behavior is parser-, syntax-, codegen-, or preprocessor-local and a
   legacy focused lane already fits, add the small targeted test there first.
2. If the behavior belongs to one of the canonical compiler buckets, prefer
   `tests/final/`.
3. If the behavior requires compile/link/runtime artifact handling, prefer
   `tests/binary/`.
4. If the behavior is not yet stable enough for promotion, keep it in
   `tests/final/probes/`.
5. If the bug was found in a real program, add the permanent minimized
   regression to the owning stable lane and keep the real-project stage only as
   an additional canary.

## Command Ladder

Use the smallest command that proves the change, then widen outward.

| Situation | Recommended Command |
|---|---|
| compiler still builds | `make` |
| broad legacy sanity pass | `make test` |
| one canonical test | `make final-id ID=<test_id>` |
| one canonical bucket | `make final-bucket BUCKET=<bucket>` |
| one canonical manifest shard | `make final-manifest MANIFEST=<manifest>.json` |
| runtime-heavy canonical slice | `make final-runtime` |
| one binary/runtime test | `make test-binary-id ID=<test_id>` |
| broader binary/runtime pass | `make test-binary` |
| probe-only blocker collection | `python3 tests/final/probes/run_probes.py` |
| real-project canary stage | run the matching `tests/real_projects/runners/run_project_*_tests.py` stage |
| checkpoint integration run | `make final` plus any relevant binary or real-project lane |

For the day-to-day stopping rules and tiered command ladder, use
`docs/compiler_test_confidence_tiers.md`.

## What A Failure Usually Means

At a high level:

- parser or syntax lane failure usually points to front-end structure or recovery behavior
- `final` non-runtime failure usually points to front-end, semantic, or lowering behavior depending on oracle type
- `binary` failure usually points to compile, link, runtime, ABI, or host-environment interaction
- probe `BLOCKED` status means the repro still captures a real gap or unstable frontier
- real-project stage failure means the compiler is not yet trustworthy for that practical integration level

For exact contribution workflow and expected evidence, use
`docs/compiler_test_workflow_guide.md` and
`docs/compiler_test_failure_taxonomy.md`.

## Public vs Private Boundary

Public docs should explain the architecture, commands, and stable contribution
flow.

Private maintainer docs are the correct place for:

- wave-by-wave execution logs
- active blocker ledgers
- promotion batches
- internal triage notes
- in-flight implementation sequencing

## Related References

- `docs/compiler_test_system_rearchitecture_context.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/compiler_test_regression_intake.md`
- `docs/compiler_test_confidence_tiers.md`
- `docs/compiler_test_coverage_blueprint.md`
- `docs/validation_workflow.md`
- `tests/final/README.md`
- `tests/final/00-harness.md`
- `tests/real_projects/README.md`
