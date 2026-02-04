# Harness Expectations

## Goal
Define how this suite executes independently from existing tests and how
expected results are recorded.

## Directory Layout
- `cases/`: input C files for this suite.
- `expect/`: per-test expectations (tokens/ast/diag/ir/out).
- `meta/`: metadata and registry (feature tags, skips, status).

## Naming Convention
Each test case uses a stable id that starts with the bucket number:

- Input: `cases/01__macro_line_directives.c`
- Expectations: `expect/01__macro_line_directives.ast`
- Metadata entry id: `01__macro_line_directives`

## Expected Outputs
- Tokens: for lexer/preprocessor tests that validate token streams.
- AST/Type: for parser and semantic tests (canonical type printing).
- Diagnostics: for error cases and recovery behavior.
- IR: for codegen validation (pattern checks or full IR match).
- Runtime: for compile-and-run tests with expected stdout/stderr/exit code.

## Metadata Schema (`meta/index.json`)
```
{
  "version": 1,
  "suite": "final",
  "tests": [
    {
      "id": "01__macro_line_directives",
      "bucket": "translation-phases",
      "input": "cases/01__macro_line_directives.c",
      "expects": ["expect/01__macro_line_directives.ast"],
      "tags": ["macro", "line-mapping"],
      "requires": [],
      "status": "ok"
    }
  ]
}
```

## Harness Notes
- AST/diag goldens should mirror the existing `tests/spec/run_ast_golden.sh`
  extraction format for compatibility.
- Token and IR expectations can be added once stable output is available.
- Keep this suite hermetic and runnable via a single harness entry point.

## Running
- `make final` runs the suite against `tests/final/meta/index.json`.
- `make final-update` regenerates `.ast` and `.diag` expectations from current output.
- `FINAL_FILTER=01__line_directive_diag_location make final` runs one test.
- Token expectations use `--dump-tokens` on the compiler when needed.
- IR expectations use `--dump-ir` on the compiler when needed.
