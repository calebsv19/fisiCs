# Translation Phases & Pipeline Integrity

## Scope
Validate phase ordering, file boundaries, macro-driven parse changes, and
stable outputs across identical inputs.

## Validate
- Deterministic AST/IR for identical inputs.
- Correct line/column after macro expansion and #line directives.
- Macro expansion affects parsing shape as required by C99.

## Acceptance Checklist
- Same input yields identical AST/diag output across runs.
- Diagnostics report correct source line/column after macro expansion.
- #line directives override diagnostic locations as expected.
- Macro expansion order can change parse shape without losing tokens.
- Include boundaries are reflected in diagnostic file paths.

## Test Cases (initial list)
1) `01__macro_changes_parse_shape`
   - Macro expands to tokens that change operator precedence or grouping.
   - Expect: AST shape changes to match expanded tokens.
2) `01__line_directive_diag_location`
   - Force an error after a `#line` directive.
   - Expect: diagnostics use the overridden line/column.
3) `01__line_directive_macro_filename`
   - `#line` uses a macro-expanded filename.
   - Expect: diagnostics use the macro-provided filename/line.
4) `01__macro_expansion_line_map`
   - Macro expands to code on a different line and triggers a diagnostic.
   - Expect: diagnostics map to the invocation site.
5) `01__include_boundary_diag_file`
   - Error inside an included header.
   - Expect: diagnostic file path is the header.
6) `01__deterministic_ast_output`
   - Same input compiled twice.
   - Expect: identical AST output text.
7) `01__macro_expansion_order`
   - Nested macros where expansion order changes the token stream.
8) `01__line_directive_include`
   - #line inside includes remaps source locations.
   - Expect: output token stream matches C99 rescan rules.

## Expected Outputs
- AST/Diagnostics goldens for parsing/semantic checks.
- Tokens (optional) for expansion order checks once token dump is available.
