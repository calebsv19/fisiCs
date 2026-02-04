# Unvalidated Behavior (needs targeted `make run` checks)

These items are not yet explicitly validated via `include/test.txt`/`make run` and should be exercised with focused snippets.

====== Lexer, literals, and tokens ======
- Integer constants: octal/hex with suffix combos and overflow diagnostics.
- Multi-character character constants and escape sequences edge cases.
- Wide/UTF prefixed literals (`L`, `u`, `U`, `u8`) if intended.
- Comment edge cases and line splicing (`\` newline) across tokens.
- Digraphs/alternative tokens (`<:`, `:>`, `%:`, `%:%:`) if intended.

====== Preprocessor ======
- Macro self-expansion suppression and re-scan behavior.
- `defined` operator in macro-expanded contexts.
- Empty `__VA_ARGS__` in variadic macros without GNU extension reliance.
- `#pragma once` behavior if supported.
- `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__` expansions.
- `#include` of macro-generated `<...>` vs `"..."` forms.

====== Declarations, types, and qualifiers ======
- Abstract declarators in casts and `sizeof` (e.g., `sizeof(int (*)[3])`).
- Function pointer typedef chains and redeclaration compatibility checks.
- VLA qualifiers in parameter lists and `static` size checks.
- `restrict` semantics and propagation (if diagnostics are planned).
- Bitfield width constraints and signedness edge cases.

====== Expressions and operators ======
- Cast vs parenthesized expression ambiguity in complex cases.
- Comma/ternary precedence edge cases with nested expressions.
- `sizeof` on qualified typenames and compound literals in expressions.
- `&&`/`||` short-circuit side effects in nested chains (codegen correctness).

====== Lvalues, rvalues, and conversions ======
- Modifiable lvalue rules for `const` struct fields and nested aggregates.
- `&` restrictions (bitfields, register, temporaries).
- Pointer/integer comparison diagnostics for non-null constant cases.
- Usual arithmetic conversions for mixed signed/unsigned widths beyond `int`.

====== Initializers and object layout ======
- Designator reset rules across nested aggregates and mixed braces.
- Brace elision behavior in multi-dimensional aggregates.
- String initializer length and truncation diagnostics.
- Static storage initialization requiring constant expressions (strict enforcement).
- Struct/union padding/offset checks against target layout.

====== Statements and control flow ======
- `switch` duplicate case detection with constant expressions.
- `goto` across VLA scope or into declarations with nontrivial init.
- Label redeclaration/forward declaration interactions.
- `break`/`continue` misuse in nested constructs recovery behavior.

====== Scoping, linkage, and redeclarations ======
- Tentative definitions across multiple translation units (if tracked).
- `static` vs `extern` mismatches across separate declarations in headers.
- Tag namespace shadowing interactions with typedef names.
- Function prototype vs K&R-style compatibility diagnostics.

====== Functions and calls ======
- Default argument promotions in non-prototype calls (if supported).
- Variadic function argument type checks vs default promotions.
- Return-by-value of large structs and ABI-sensitive cases (if applicable).
- Function pointer comparisons and assignments with differing prototypes.

====== Codegen and lowering ======
- Volatile load/store semantics in IR.
- Switch lowering with fallthrough and default-only cases in IR.
- Ternary lowering with pointer/struct operands in IR.
- VLA stack allocation/deallocation strategy and lifetime.

====== Runtime surface and builtins ======
- Coverage for additional system headers (e.g., `stdarg.h`, `stddef.h` edge cases).
- Builtin stubs for platform headers beyond current secure-check intrinsics.

====== Diagnostics ======
- Recovery in mixed declaration/statement blocks with cascading errors.
- Error suppression to avoid duplicate diagnostics across passes (non-env).
- Precise source location mapping after macro expansion in diagnostics.
- Consistent hint/note emission for common parse/semantic failures.

====== External project validations ======
- Broader compilation of external codebases with nested includes and macros.
- System header parsing with SDKs beyond macOS (if relevant).
