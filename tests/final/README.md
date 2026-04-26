# C99 Behavior Test Suite (Final)

This directory is the canonical bucketed compiler-behavior suite for `fisiCs`.
Each file below describes one public bucket reference: what it covers, what
kind of oracle it uses, and when contributors should route a stable regression
into it.

Quick run entrypoints:
- `make final`: full suite (checkpoint/integration run).
- `make final-id ID=<test_id>`: one test id.
- `make final-wave WAVE=<n>`: one runtime wave shard (`14-runtime-surface` default).
- `make final-manifest MANIFEST=<manifest.json>`: one manifest shard.
- `make final-prefix PREFIX=<id_prefix>`: one id-prefix slice.
- `make final-runtime`: bucket-14 runtime slice.
- `make final-bucket BUCKET=torture-differential`: bucket-15 torture slice.

## Files
- `00-harness.md`: How this suite is run and how expected outputs are stored.
- `01-translation-phases.md`: Translation phases and pipeline integrity.
- `02-lexer.md`: Tokenization correctness.
- `03-preprocessor.md`: Macro expansion, includes, and #if logic.
- `04-declarations.md`: Declarators, type specifiers, and aggregates.
- `05-expressions.md`: Operator precedence and expression parsing.
- `06-lvalues-rvalues.md`: Addressability and modifiable lvalues.
- `07-types-conversions.md`: Promotions and usual arithmetic conversions.
- `08-initializers-layout.md`: Initializers and object layout rules.
- `09-statements-controlflow.md`: Control flow statements and scoping.
- `10-scopes-linkage.md`: Namespaces, linkage, and redeclarations.
- `11-functions-calls.md`: Calls, prototypes, and variadics.
- `12-diagnostics-recovery.md`: Error recovery and diagnostic quality.
- `13-codegen-ir.md`: IR and lowering semantics (when enabled).
- `14-runtime-surface.md`: Minimal libc surface and header expectations.
- `15-torture-differential.md`: Stress/differential torture coverage and expansion plan.
- `15-conformance-strategy.md`: Feature matrix and intentional support gaps.
- `probes/README.md`: Triage repro fixtures and probe runner (not in `make final`).

Related public references:
- `docs/compiler_test_architecture.md`
- `docs/compiler_test_workflow_guide.md`
- `docs/compiler_test_failure_taxonomy.md`
- `docs/compiler_test_regression_intake.md`
- `docs/compiler_test_confidence_tiers.md`

Detailed promotion batches, probe queues, and expansion sequencing are
maintainer-facing and intentionally kept out of this public README.
