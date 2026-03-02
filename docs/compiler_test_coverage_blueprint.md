# Compiler Test Coverage Blueprint

This document turns the current compiler test-system re-architecture brief into
a concrete coverage and harness plan for systematic C99 to C17 validation.

It is intentionally grounded in the current repository state:

- `make test` exists today, but it is assembled from hardcoded target lists in
  `makefile`.
- The legacy suite is mostly shell-script driven under `tests/parser/`,
  `tests/syntax/`, `tests/codegen/`, `tests/preprocessor/`, and `tests/spec/`.
- A newer metadata-driven suite already exists under `tests/final/` with
  `tests/final/run_final.py` and `tests/final/meta/index.json`.
- `make final-update` currently rewrites expectations broadly, which conflicts
  with the required targeted-only golden regeneration model.

The plan below uses `tests/final/` as the migration anchor instead of inventing
a second new framework.

## Primary Direction

The system should evolve into one canonical metadata-driven harness with these
properties:

- `make test` remains the top-level entrypoint.
- Layered targets (`make test-lexer`, `make test-preprocessor`, etc.) select
  metadata buckets, not hardcoded filenames.
- Default runs are read-only against expectations and write all generated
  artifacts to `build/tests/`.
- Regeneration is explicit, per-test, and confirm-gated.
- Runtime and differential checks become first-class instead of ad hoc shell
  scripts.
- Existing legacy shell tests stay in place only until their coverage is moved
  into the metadata suite.

## Current Gaps That Matter

The current repo already has useful coverage, but the infrastructure still has
structural weaknesses:

- Hardcoded target membership in `makefile` makes silent omissions easy.
- `tests/final/run_final.py` supports metadata, but currently compares only
  `.ast`, `.diag`, `.tokens`, and `.ir`, with no runtime oracle model yet.
- `make final-update` can rewrite many expected files in one pass, which is the
  exact golden-poisoning path we need to remove.
- The current `tests/final` runner writes accepted output directly into
  `tests/final/expect/` during update mode instead of staging under
  `build/tests/`.
- Differential reference behavior against `clang` is not yet part of the
  `tests/final` contract.

## Canonical Test Layout

The repo already has a good semantic split in `tests/final/`. Keep that as the
canonical suite, but make it the authoritative source of truth:

- `tests/final/cases/`: source inputs
- `tests/final/expect/`: checked-in expectations
- `tests/final/meta/`: metadata registry
- `build/tests/final/<test-id>/`: all generated outputs, logs, temp files, and
  diff artifacts

Recommended stable buckets, aligned with the existing suite documents:

1. `translation-phases`
2. `lexer`
3. `preprocessor`
4. `declarations`
5. `expressions`
6. `lvalues-rvalues`
7. `types-conversions`
8. `initializers-layout`
9. `statements-controlflow`
10. `scopes-linkage`
11. `functions-calls`
12. `diagnostics-recovery`
13. `codegen-ir`
14. `runtime-surface`
15. `conformance-strategy`
16. `torture-differential`
17. `regression`

The current numbered `tests/final/*.md` files already cover buckets 1 through
15. Add explicit metadata buckets for `torture-differential` and `regression`
rather than burying those concerns inside other categories.

## Coverage Map By Compiler Layer

### 1. Translation Phases and Lexer

This layer must prove token boundaries are correct before parsing is trusted.

Mandatory coverage:

- Keywords across C99 and supported C11/C17 additions
- Identifiers, including long identifiers and any supported universal character
  names
- Integer literals: decimal, octal, hex, suffix permutations, overflow
  diagnostics
- Floating literals: decimal and hex forms, exponent variants, suffixes
- Character constants: narrow, wide, UTF forms if supported, escapes, and
  multi-character constants
- String literals: concatenation, escapes, wide and UTF prefixes
- Full punctuator and operator surface, including digraphs
- Comments, comment adjacency, and comment interaction with preprocessing
- Preprocessing-token stream checks distinct from post-translation tokens

Edge cases:

- Trigraphs if supported, otherwise explicit unsupported diagnostics
- Backslash-newline line splicing
- Extremely long identifiers
- Unicode identifier acceptance or explicit rejection
- Invalid escape handling
- Tokenization around macro boundaries and adjacent literals

Assertion modes:

- `.tokens` for token stream stability
- `.diag` for malformed literal and escape diagnostics
- `.ast` only when translation-phase behavior changes parse shape

### 2. Preprocessor

This layer needs dense coverage because it is a common source of fake
conformance.

Mandatory coverage:

- Object-like and function-like macros
- Nested expansion and rescan behavior
- Recursive expansion blocking rules
- Stringification (`#`)
- Token pasting (`##`)
- Variadic macros, including empty variadic arguments
- Macro-to-nothing expansions
- Macros that emit syntax fragments
- `#if` constant expressions, `defined`, nested conditional ladders
- Include lookup for `"file.h"` and `<file.h>`
- Include guards, repeated includes, and cycle handling

Edge cases:

- Macro expansion inside macro arguments
- Whitespace-sensitive stringization cases
- Macro expansion ordering
- Token pasting that forms numeric literals or operators
- `#undef` interactions
- Short-circuit correctness in `#if`

Assertion modes:

- `.tokens` for preprocessor output
- `.diag` for line/file mapping and directive errors
- Optional reference comparison against `clang -E` when available

### 3. Parsing and Grammar Completeness

The parser plan should track grammar productions, not just random examples.

Mandatory coverage:

- Storage classes, qualifiers, `_Atomic`, `inline`, `_Thread_local`
- Alignment specifiers and typedef interactions
- Integer, floating, complex, aggregate, enum, incomplete, and function types
- Complex declarators: arrays, pointers, arrays of pointers, function pointers,
  abstract declarators, VLAs
- Expression precedence, cast ambiguity, compound literals, `sizeof`,
  `_Alignof`, `_Generic`, ternary edge cases
- Statements: blocks, `if`/`else`, `switch`, all loop forms, `goto`, labels,
  `break`, `continue`, `return`
- Initializer forms: designated initializers, nested designators, brace elision,
  zero initialization

Edge cases:

- Anonymous and nested structs/unions
- Self-referential types
- Flexible array members
- Duplicate `case` detection
- Dangling `else`
- Mixed designated and non-designated initializers

Assertion modes:

- `.ast` for parse shape
- `.diag` for rejected constructs and ambiguity resolution errors

### 4. Semantic Analysis

This is where correctness bugs become subtle and dangerous.

Mandatory coverage:

- File, block, and prototype scope
- Shadowing and tentative definitions
- Integer promotions and usual arithmetic conversions
- Pointer, array, and function decay rules
- Lvalue versus rvalue legality
- Const and volatile qualifier enforcement
- Prototype matching and argument promotion
- Variadic call checks
- Return type enforcement
- Struct and union member lookup
- Enum constant evaluation
- Constant-expression rules for array sizes, case labels, and static
  initializers
- Linkage rules, `static` and `extern` interactions, and multiple-definition
  diagnostics

Edge cases:

- Compile-time division by zero if folded
- Invalid shift widths
- Incompatible pointer types
- Multiple-error recovery without crashes

Assertion modes:

- `.diag` for semantic failures
- `.ast` or normalized semantic dumps for stable type-state validation when the
  compiler exposes them

### 5. Codegen, IR, and Executable Behavior

IR checks are useful, but executable behavior is the stronger oracle when safe.

Mandatory coverage:

- Arithmetic lowering, signed versus unsigned behavior, comparisons, and logical
  ops
- Stack allocation and alignment
- Struct and array layout matching ABI expectations
- Pointer arithmetic and dereference lowering
- Control-flow lowering: branches, short-circuit logic, loops, switches, phi
  formation
- Function ABI behavior: arguments, returns, varargs, struct returns
- Recursion, large stack frames, nested scopes, function pointers

Assertion modes:

- `.ir` for normalized structural checks where the IR is stable enough
- Runtime `stdout`, `stderr`, and exit-code checks for behavior
- Differential runtime comparison against `clang` for UB-free tests

### 6. Standard Conformance Surface

Track supported language surface explicitly, not informally.

Required matrix:

- C99: `inline`, VLAs, designated initializers, complex numbers, `_Bool`,
  `stdint`-related behavior
- C11/C17: `_Static_assert`, `_Alignas`, `_Alignof`, `_Generic`,
  `_Thread_local`, and atomics if supported
- Explicit unsupported markers for any intentionally missing features

Every feature should be classified as one of:

- Supported and tested
- Supported partially and gated
- Intentionally unsupported with stable diagnostics
- Unknown and not yet claimed

### 7. Torture, Differential, and Robustness

This is the confidence layer that catches regressions outside curated unit
coverage.

Required categories:

- Differential compile/run checks against `clang`
- Optional differential checks against `gcc` as a secondary reference
- Fuzz-generated valid programs (for example, `csmith`) in a non-default target
- Real-world corpus compile smoke tests
- Malformed-input stress tests that must not crash the compiler

Safety rules:

- Differential tests must be marked `ub: false` before behavior comparison
- UB-prone or target-sensitive tests must skip diff or compare only compile
  success and diagnostics
- Torture and fuzz targets should be opt-in (`make test-torture`) rather than
  part of the fast default suite

## Assertion Strategy By Test Category

Each test in metadata should declare one primary category:

- `compile_ok`
- `compile_fail`
- `runtime`
- `differential`
- `regression`

And each test should declare exactly which oracle applies:

- Token stream
- AST dump
- IR dump
- Diagnostics
- Runtime output and exit code
- Differential runtime match

This keeps expectations explicit and prevents ambiguous pass conditions.

## Metadata Expansion Needed

The existing `tests/final/meta/index.json` is a good start, but it needs a
slightly richer schema to support the full plan:

- `id`
- `bucket`
- `category`
- `standard`
- `inputs`
- `expects`
- `expect_exit`
- `expect_stdout`
- `expect_stderr`
- `expected_diag`
- `args`
- `run`
- `ub`
- `requires`
- `skip_if`
- `bug` for regression linkage
- `allow_reference` for differential eligibility

Important policy:

- `status` may describe maturity (`ok`, `known-fail`, `skip`), but it must not
  silently downgrade a failure into a pass.
- A `known-fail` should be counted separately and should still fail the suite
  unless the target explicitly opts into an allowed-xfail mode.

## Harness Changes Required

The existing `tests/final/run_final.py` should be evolved, not replaced.

Required behavior changes:

- Discover tests from metadata and bucket filters only
- Write all actual outputs to `build/tests/final/<id>/`
- Compare checked-in expectations against staged actual outputs
- Print per-test `PASS`, `FAIL`, or `SKIP`
- Emit a final summary with passed, failed, and skipped counts
- Detect `clang` at runtime and enable differential mode only when present
- Treat missing expectation files as failures unless metadata explicitly marks an
  oracle optional

Required make targets:

- `make test`
- `make test-lexer`
- `make test-preprocessor`
- `make test-parser`
- `make test-semantic`
- `make test-codegen`
- `make test-runtime`
- `make test-regression`
- `make test-torture`

The existing legacy targets can temporarily remain as compatibility wrappers,
but they should converge on the same metadata harness.

## Golden Safety Rules For This Repo

The current `final-update` behavior is too broad. Replace it with guarded,
targeted regeneration:

- `make regen TEST=<id> CONFIRM=YES`
- `make regen FILE=<path> CONFIRM=YES`
- `make regen-preview TEST=<id>`

Repo-specific rules:

- `final-update` should be removed or changed into a guarded alias that aborts
  unless `TEST` or `FILE` is provided and `CONFIRM=YES`.
- Regeneration must stage candidate outputs under `build/tests/final/<id>/regen`
  before overwriting anything.
- On regen, print changed files and unified diffs.
- For differential-eligible tests, verify the new result still matches `clang`
  before replacing the checked-in expectation.
- Regression tests must never be mass-updated.

## Rollout Order

### Phase 1: Lock the harness contract

- Extend `tests/final/meta/index.json` schema
- Make `tests/final/run_final.py` non-destructive by default
- Move actual-output staging into `build/tests/`
- Add bucket filtering that maps directly to future `make test-*` targets

### Phase 2: Migrate existing coverage into metadata

- Mirror current parser, syntax, codegen, preprocessor, and spec checks into
  `tests/final`
- Keep the old shell tests as parity references during migration
- Remove hardcoded filename lists from `makefile` once metadata coverage is
  equivalent

### Phase 3: Fill edge-case gaps

- Use the layer coverage map above as the checklist
- Add missing lexer, preprocessor, grammar, semantic, and runtime cases
- Add explicit unsupported-feature diagnostics for non-implemented C11/C17
  corners

### Phase 4: Add differential and stress layers

- Introduce `clang`-backed runtime comparisons
- Add malformed-input no-crash tests
- Add opt-in corpus and fuzz targets

### Phase 5: Freeze regressions

- Every bug fix gets a stable `regression` test with a bug id
- Regression expectations are never bulk-regenerated

## Practical Use Of This Blueprint

When new specs arrive, classify each requested case in three dimensions:

1. Which bucket it belongs to
2. Which oracle should judge it
3. Whether it is safe for differential comparison

If a proposed test does not answer those three questions, it is not ready to
add yet.
