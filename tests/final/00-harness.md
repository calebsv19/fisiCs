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

## Metadata Schema (`meta/index.json` + bucket manifests)
`meta/index.json` is the manifest registry. It can either contain tests directly
(legacy mode) or list shard files under `meta/` (current mode).

Current preferred layout:
```
{
  "version": 1,
  "suite": "final",
  "manifests": [
    "01-translation-phases.json",
    "02-lexer.json",
    "03-preprocessor.json"
  ]
}
```

Each shard file uses the per-test schema below:
```
{
  "version": 1,
  "suite": "final",
  "tests": [
    {
      "id": "01__macro_line_directives",
      "bucket": "translation-phases",
      "input": "cases/01__macro_line_directives.c",
      "args": ["-Itests/final/cases/headers"],
      "env": {"FISICS_MACRO_EXP_LIMIT": "2"},
      "expects": ["expect/01__macro_line_directives.ast"],
      "tags": ["macro", "line-mapping"],
      "capture_frontend_diag": false,
      "allow_nonzero_exit": false,
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
- `make final` runs the suite against `tests/final/meta/index.json`, which now
  acts as a registry for one or more manifest shard files.
- `make final-update` regenerates `.ast` and `.diag` expectations from current output.
- `FINAL_FILTER=01__line_directive_diag_location make final` runs one test.
- Token expectations use `--dump-tokens` on the compiler when needed.
- IR expectations use `--dump-ir` on the compiler when needed.
- `capture_frontend_diag: true` records early `Error:` / `Warning:` lines that
  appear before the semantic section, including `path:line:col: error:` style
  front-end diagnostics.
- `allow_nonzero_exit: true` permits token/diag oracle checks to proceed even if
  later phases intentionally fail the compile.
- `args: [...]` passes per-test compiler flags before input files (for example,
  explicit layered `-I` include path tests).
- `env: {...}` sets per-test environment variables (for example macro-expansion
  limit stress checks).
