# C99 Behavior Test Suite (Final)

This directory is a clean, separate test suite for full C99 behavior coverage.
Each file below describes one bucket, what it covers, and how the tests should
be validated (tokens, AST, diagnostics, IR, or runtime output).

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
- `15-conformance-strategy.md`: Feature matrix and intentional gaps.
- `probes/README.md`: Triage repro fixtures and probe runner (not in `make final`).
