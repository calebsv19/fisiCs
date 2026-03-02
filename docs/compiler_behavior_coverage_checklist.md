# Compiler Behavior Coverage Checklist

This document is the working execution checklist for the compiler test upgrade.

Use it as the per-bucket validation tracker while migrating the suite toward
phase-complete C99 to C17 behavior coverage. The workflow is:

1. Pick one bucket.
2. Add or run all tests for that bucket.
3. Record which cases pass, fail, or are unsupported.
4. Collect all failures for that bucket before fixing behavior.
5. Fix the bucket as a group.
6. Re-run the bucket and close it only when the full section is stable.

This is the operational companion to:

- `docs/compiler_test_system_rearchitecture_context.md`
- `docs/compiler_test_coverage_blueprint.md`

## How To Use This Checklist

Every checklist item below is a feature family, not a single test.

Each feature family must eventually have:

- At least one valid test
- At least one invalid or negative test where applicable
- At least one edge or stress variant

As tests are implemented, track them in this format:

| Field | Meaning |
| --- | --- |
| Feature | Language or compiler behavior being validated |
| Bucket | Test harness bucket (`lexer`, `preprocessor`, `runtime`, etc.) |
| Valid | Positive test coverage exists and passes |
| Negative | Invalid-input or rejection coverage exists and passes |
| Edge | Boundary or stress variant exists and passes |
| Status | `unstarted`, `in_progress`, `blocked`, `passing`, `unsupported` |
| Oracle | `tokens`, `diag`, `ast`, `ir`, `runtime`, `differential` |
| Planned Tests | Intended test ids or filenames |
| Failures Seen | Current observed compiler gaps or regressions |
| Notes | Extra constraints, UB tags, ABI sensitivity, or follow-up |

Recommended pass discipline per bucket:

- Do not mark a bucket complete because a few representative tests pass.
- Run the whole bucket and log all failures first.
- Fix behavior only after the failure set is visible.
- Re-run the full bucket after fixes, not just the changed case.

## Tracking Conventions

- Mark unsupported features explicitly. Do not leave them ambiguous.
- Tag UB and implementation-defined tests so they can be excluded from strict
  differential comparison.
- Diagnostics should be judged by category and location, not full text.
- Runtime behavior is preferred over textual IR matching when both are viable.
- Until concrete test files exist, use planned ids in the form
  `tests/final/cases/<bucket>__<feature>__<variant>.c`.

## Phase 1 — Lexical Analysis (Tokenization)

Primary bucket:

- `lexer`

Primary oracles:

- `tokens`
- `diag`

### 1.1 Identifiers

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic ASCII identifiers | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__identifiers__ascii_valid` | | |
| Leading underscore | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__identifiers__leading_underscore` | | |
| Multiple underscores | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__identifiers__multi_underscore` | | |
| Extremely long identifiers | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__identifiers__long_identifier` | | Stress path |
| Keywords vs identifiers | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__identifiers__keyword_boundary` | | |
| Universal character names | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__identifiers__ucn_support` | | Mark unsupported if not implemented |
| Identifier shadowing via macro expansion | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__identifiers__macro_shadowing` | | Cross-phase with preprocessor |

### 1.2 Keywords (C99 + C11/C17)

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| All standard keywords recognized | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__keywords__recognition_matrix` | | Split by standard level if needed |
| Misuse as identifiers rejected | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `02__keywords__identifier_reject` | | |

### 1.3 Integer Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Decimal, octal, hexadecimal | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__int_literals__base_forms` | | |
| `0`-prefixed octal edge cases | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__int_literals__octal_edges` | | |
| Hex uppercase and lowercase | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__int_literals__hex_case` | | |
| Suffix combinations | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__int_literals__suffix_matrix` | | `U`, `L`, `LL`, `UL`, `ULL` |
| Overflow detection | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `02__int_literals__overflow` | | Diagnostic vs accepted wrap must be explicit |
| Large boundary constants | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__int_literals__boundaries` | | `INT_MAX`, `UINT_MAX` equivalents |
| Invalid suffix combinations | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `02__int_literals__invalid_suffix` | | |

### 1.4 Floating Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Decimal floats | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__float_literals__decimal` | | |
| Scientific notation | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__float_literals__scientific` | | |
| Hex floats | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__float_literals__hex` | | Mark unsupported if rejected intentionally |
| Suffixes `f/F/l/L` | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__float_literals__suffixes` | | |
| Boundary forms (`.5`, `5.`, `5e+3`) | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__float_literals__edge_forms` | | |
| Invalid exponent forms | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `02__float_literals__invalid_exponent` | | |
| Precision boundaries | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__float_literals__precision_boundary` | | |

### 1.5 Character Constants

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Standard char constants | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__char_literals__basic` | | |
| Escape sequences | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__char_literals__escapes` | | |
| Octal and hex escapes | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__char_literals__numeric_escapes` | | |
| Multi-character constants | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__char_literals__multichar` | | Implementation-defined value |
| Wide char (`L'a'`) | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__char_literals__wide` | | |
| UTF-8/16/32 forms | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__char_literals__utf` | | Mark unsupported if absent |
| Overflow in escape sequences | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `02__char_literals__escape_overflow` | | |

### 1.6 String Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Normal strings | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__string_literals__basic` | | |
| Concatenated adjacent strings | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__string_literals__concat` | | |
| Wide and UTF strings | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__string_literals__wide_utf` | | |
| Embedded null bytes | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__string_literals__embedded_null` | | |
| Escape correctness | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__string_literals__escapes` | | |
| Extremely long strings | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__string_literals__long` | | Stress path |

### 1.7 Operators and Punctuators

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| All unary operators | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__unary` | | |
| All binary operators | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__binary` | | |
| Ternary operator tokenization | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__ternary_tokens` | | |
| Compound assignments | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__compound_assign` | | |
| Pre/post increment | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__inc_dec` | | |
| Arrow operator | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__arrow` | | |
| Digraphs | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__operators__digraphs` | | Mark unsupported if absent |
| Trigraphs | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__operators__trigraphs` | | Mark unsupported if absent |
| Token boundary correctness | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__operators__token_boundaries` | | Includes adjacent punctuators |

### 1.8 Comments

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Single-line comments | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__comments__single_line` | | |
| Multi-line comments | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__comments__multi_line` | | |
| Nested-like patterns | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__comments__nested_like` | | |
| Comments inside macros | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__comments__macro_context` | | Cross-phase with preprocessor |
| Backslash-newline splicing | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `02__comments__line_splice` | | |

### 1.9 Preprocessing Tokens

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Correct tokenization before macro expansion | `lexer` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `02__pp_tokens__pre_expansion_stream` | | Distinct from final parser tokens |

## Phase 2 — Preprocessor

Primary bucket:

- `preprocessor`

Primary oracles:

- `tokens`
- `diag`
- `differential` (optional via `clang -E`)

### 2.1 Object-Like Macros

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic replacement | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__object_macro__basic` | | |
| Recursive blocking behavior | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__object_macro__recursive_block` | | |
| Redefinition warnings or errors | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `03__object_macro__redefinition` | | Policy must be explicit |

### 2.2 Function-Like Macros

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Parameter substitution | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__fn_macro__params` | | |
| Nested macros in arguments | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__fn_macro__nested_args` | | |
| Empty argument lists | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__fn_macro__empty_args` | | |
| Variadic macros | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__fn_macro__variadic` | | |
| `__VA_ARGS__` | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__fn_macro__va_args` | | |
| Token pasting (`##`) | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__fn_macro__paste` | | |
| Stringification (`#`) | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__fn_macro__stringify` | | |
| Expansion order correctness | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__fn_macro__expansion_order` | | |

### 2.3 Conditional Compilation

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `#if` constant expressions | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__if__constant_expr` | | |
| `defined` operator | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__if__defined` | | |
| Nested conditionals | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__if__nested` | | |
| Short-circuit logic correctness | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__if__short_circuit` | | |
| Edge numeric expressions | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__if__numeric_edges` | | |
| Undefined macro in `#if` | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__if__undefined_macro` | | |

### 2.4 Include Handling

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `"file.h"` vs `<file.h>` | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__include__quote_vs_angle` | | |
| Include guards | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__include__guards` | | |
| Multiple include idempotence | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__include__idempotence` | | |
| Include cycles detection | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `03__include__cycle` | | |
| Missing include diagnostics | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `03__include__missing` | | |

### 2.5 Edge Cases

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Macro expanding to nothing | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__edge__macro_to_nothing` | | |
| Macro generating partial tokens | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__edge__partial_tokens` | | |
| Whitespace sensitivity | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens` | `03__edge__whitespace` | | |
| Line continuation behavior | `preprocessor` | [ ] | [ ] | [ ] | `unstarted` | `tokens`, `diag` | `03__edge__line_continuation` | | |

## Phase 3 — Declarations and Types (Parsing)

Primary buckets:

- `declarations`
- `parser`

Primary oracles:

- `ast`
- `diag`

### 3.1 Storage Classes

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `static` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__storage__static` | | |
| `extern` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__storage__extern` | | |
| `auto` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__storage__auto` | | |
| `register` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__storage__register` | | |
| `_Thread_local` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__storage__thread_local` | | Mark unsupported if absent |

### 3.2 Type Qualifiers

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `const` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__qualifiers__const` | | |
| `volatile` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__qualifiers__volatile` | | |
| `restrict` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__qualifiers__restrict` | | |
| `_Atomic` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__qualifiers__atomic` | | Mark unsupported if absent |

### 3.3 Primitive Types

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `char` signedness behavior | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `04__primitive__char_signedness` | | Implementation-defined |
| `short`, `int`, `long`, `long long` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__primitive__integers` | | |
| `float`, `double`, `long double` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `04__primitive__floating` | | |
| `_Bool` | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `04__primitive__bool` | | |
| Complex types | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__primitive__complex` | | Mark unsupported if absent |

### 3.4 Structs

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic struct | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__struct__basic` | | |
| Nested struct | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__struct__nested` | | |
| Anonymous struct | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__struct__anonymous` | | |
| Self-referential struct | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__struct__self_ref` | | |
| Flexible array member | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__struct__flex_array` | | |
| Bitfields | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `04__struct__bitfields` | | ABI-sensitive |
| Duplicate member names | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `04__struct__duplicate_member` | | |

### 3.5 Unions

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Basic union | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__union__basic` | | |
| Anonymous union | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__union__anonymous` | | |
| Nested union | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__union__nested` | | |
| Overlapping member correctness | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `04__union__overlap` | | ABI-sensitive |

### 3.6 Enums

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Explicit enumerator values | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__enum__explicit_values` | | |
| Implicit increment behavior | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `04__enum__implicit_increment` | | |
| Negative values | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `04__enum__negative_values` | | |
| Large values | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__enum__large_values` | | |
| Duplicate enumerators | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `04__enum__duplicate` | | |

### 3.7 Declarator Complexity

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Pointer to function | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__declarator__ptr_to_fn` | | |
| Function returning pointer | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__declarator__fn_returns_ptr` | | |
| Pointer to array | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__declarator__ptr_to_array` | | |
| Array of pointers | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__declarator__array_of_ptrs` | | |
| Function pointer parameters | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__declarator__fn_ptr_param` | | |
| Abstract declarators | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__declarator__abstract` | | |
| VLAs | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `04__declarator__vla` | | Mark unsupported if absent |
| Multidimensional arrays | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `04__declarator__multi_array` | | |

### 3.8 Typedef

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Typedef of complex types | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `04__typedef__complex_type` | | |
| Typedef shadowing | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `04__typedef__shadowing` | | |
| Redefinition errors | `declarations` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `04__typedef__redefinition` | | |

## Phase 4 — Expressions

Primary buckets:

- `expressions`
- `semantic`

Primary oracles:

- `ast`
- `diag`
- `runtime`
- `ir`

### 4.1 Operator Precedence

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| All precedence tiers | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__precedence__all_tiers` | | |
| Nested combinations | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__precedence__nested` | | |

### 4.2 Unary Operations

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Unary `+`, `-`, `!` | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__unary__basic` | | |
| Bitwise `~` | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__unary__bitwise_not` | | |
| Address-of | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `05__unary__address_of` | | |
| Dereference | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `05__unary__deref` | | |
| `sizeof` type vs expression | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__unary__sizeof_ambiguity` | | |
| `_Alignof` | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `diag` | `05__unary__alignof` | | Mark unsupported if absent |

### 4.3 Binary Operations

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Arithmetic | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__binary__arithmetic` | | |
| Bitwise | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__binary__bitwise` | | |
| Logical | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `ir` | `05__binary__logical` | | |
| Relational | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__binary__relational` | | |
| Equality | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__binary__equality` | | |
| Shift signed and unsigned | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `diag` | `05__binary__shift` | | UB-sensitive edges |
| Invalid shift widths | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `05__binary__invalid_shift_width` | | |

### 4.4 Ternary Operator

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Nested ternary | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__ternary__nested` | | |
| Mixed types | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `diag` | `05__ternary__mixed_types` | | |
| Lvalue cases | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `05__ternary__lvalue` | | |

### 4.5 Casts

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Explicit casts | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__casts__explicit` | | |
| Illegal casts | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `05__casts__illegal` | | |
| Cast vs parenthesis ambiguity | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `05__casts__ambiguity` | | |

### 4.6 Compound Literals

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Struct literal | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__compound_literal__struct` | | |
| Array literal | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `05__compound_literal__array` | | |
| Static storage compound literal | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `ir` | `05__compound_literal__static_storage` | | |

### 4.7 `_Generic`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `_Generic` selection | `expressions` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `05__generic__selection` | | Mark unsupported if absent |

## Phase 5 — Statements

Primary buckets:

- `statements-controlflow`
- `parser`
- `semantic`

Primary oracles:

- `ast`
- `diag`
- `runtime`
- `ir`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Expression statement | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `09__stmt__expr` | | |
| Compound block | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast` | `09__stmt__compound` | | |
| `if`/`else` and dangling else | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `09__stmt__if_else` | | |
| `switch` fallthrough | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `ir` | `09__stmt__switch_fallthrough` | | |
| Duplicate `case` detection | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__switch_duplicate_case` | | |
| Non-constant `case` label | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__switch_non_constant` | | |
| `while` | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `ir` | `09__stmt__while` | | |
| `do-while` | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `ir` | `09__stmt__do_while` | | |
| `for` loop forms | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `ir` | `09__stmt__for_forms` | | |
| Declaration in `for` initializer | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `09__stmt__for_decl_init` | | |
| `break` and `continue` legality | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `runtime` | `09__stmt__break_continue` | | |
| `goto` and labels | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `09__stmt__goto_labels` | | |
| Duplicate label error | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__duplicate_label` | | |
| `goto` across scopes | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `runtime` | `09__stmt__goto_scope_cross` | | |
| `return` basic correctness | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `09__stmt__return_basic` | | |
| Missing return | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__missing_return` | | Policy may be warning or error |
| Wrong return type | `statements-controlflow` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `09__stmt__wrong_return_type` | | |

## Phase 6 — Initializers

Primary buckets:

- `initializers-layout`
- `semantic`

Primary oracles:

- `ast`
- `diag`
- `runtime`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Scalar initialization | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `08__init__scalar` | | |
| Aggregate initialization | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `08__init__aggregate` | | |
| Designated initializers | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime`, `diag` | `08__init__designated` | | |
| Nested designated initializers | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `08__init__nested_designated` | | |
| Mixed designated and non-designated | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `08__init__mixed_designated` | | |
| Excess elements | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `08__init__excess_elements` | | |
| Missing elements and zero-fill | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `08__init__zero_fill` | | |
| Static initialization constant-expression enforcement | `initializers-layout` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `08__init__static_constexpr` | | |

## Phase 7 — Semantics

Primary buckets:

- `semantic`
- `scopes-linkage`
- `functions-calls`
- `lvalues-rvalues`

Primary oracles:

- `diag`
- `ast`
- `runtime`

### 7.1 Scope Rules

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Block scope | `scopes-linkage` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `10__scope__block` | | |
| File scope | `scopes-linkage` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `10__scope__file` | | |
| Shadowing | `scopes-linkage` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `10__scope__shadowing` | | |
| Tentative definitions | `scopes-linkage` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `10__scope__tentative_def` | | |
| Multiple definition errors | `scopes-linkage` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `10__scope__multiple_def` | | Multi-file |

### 7.2 Type Conversions

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Integer promotions | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__integer_promotions` | | |
| Usual arithmetic conversions | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__usual_arith` | | |
| Signed vs unsigned comparisons | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__signed_unsigned_compare` | | |
| Pointer conversions | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag`, `runtime` | `07__conv__pointer` | | |
| Array-to-pointer decay | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__array_decay` | | |
| Function-to-pointer decay | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__conv__function_decay` | | |

### 7.3 Lvalue and Rvalue Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Assignment target rules | `lvalues-rvalues` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `ast` | `06__lvalue__assignment_target` | | |
| Const correctness enforcement | `lvalues-rvalues` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `ast` | `06__lvalue__const` | | |
| Volatile propagation | `lvalues-rvalues` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `06__lvalue__volatile` | | |

### 7.4 Function Semantics

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Prototype matching | `functions-calls` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `ast` | `11__fn__prototype_match` | | |
| Argument count mismatch | `functions-calls` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `11__fn__arg_count_mismatch` | | |
| Variadic correctness | `functions-calls` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `runtime` | `11__fn__variadic` | | |
| Return type enforcement | `functions-calls` | [ ] | [ ] | [ ] | `unstarted` | `diag`, `runtime` | `11__fn__return_type` | | |

### 7.5 Struct and Union Semantics

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Member access correctness | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `runtime` | `07__agg__member_access` | | |
| Invalid member access | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__agg__invalid_member` | | |
| Offset correctness | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `07__agg__offsets` | | ABI-sensitive |

### 7.6 Constant Expressions

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Required in array size | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__constexpr__array_size` | | |
| Required in case labels | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__constexpr__case_label` | | |
| Required in static initializer | `semantic` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `07__constexpr__static_init` | | |

## Phase 8 — Code Generation and Runtime

Primary buckets:

- `codegen-ir`
- `runtime`
- `differential`

Primary oracles:

- `runtime`
- `ir`
- `differential`

### 8.1 Arithmetic Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Signed overflow behavior | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__arith__signed_overflow` | | UB-tag required |
| Unsigned wraparound | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__arith__unsigned_wrap` | | Safe for diff |
| Division and modulo edge cases | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `diag` | `14__arith__div_mod_edges` | | Division by zero is UB if runtime |

### 8.2 Floating-Point Correctness

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Precision behavior | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__float__precision` | | Target-specific tolerance may be needed |
| NaN propagation | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__float__nan` | | May need pattern-based compare |
| Infinity behavior | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__float__infinity` | | |

### 8.3 Control Flow

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Nested loops | `codegen-ir` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `ir` | `13__cf__nested_loops` | | |
| Short-circuit logic | `codegen-ir` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `ir`, `differential` | `13__cf__short_circuit` | | |
| Switch lowering correctness | `codegen-ir` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `ir` | `13__cf__switch_lowering` | | |

### 8.4 Memory

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Stack allocation | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `ir` | `14__mem__stack_alloc` | | |
| Struct layout correctness | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__mem__struct_layout` | | ABI-sensitive tag required |
| Array indexing | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__mem__array_indexing` | | |
| Pointer arithmetic | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__mem__pointer_arith` | | |
| Alignment correctness | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `ir` | `14__mem__alignment` | | ABI-sensitive |

### 8.5 Function Calls

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Parameter passing | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__calls__params` | | |
| Variadic calls | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__calls__variadic` | | |
| Recursion | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__calls__recursion` | | |
| Function pointers | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__calls__fn_ptr` | | |
| Struct return | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `differential` | `14__calls__struct_return` | | ABI-sensitive |

### 8.6 Stress Runtime

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Large stack frames | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__stress__large_stack` | | Stress path |
| Deep recursion | `runtime` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `14__stress__deep_recursion` | | Stress path |

## Phase 9 — Diagnostics

Primary buckets:

- `diagnostics-recovery`
- `semantic`
- `parser`

Primary oracle:

- `diag`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Syntax errors | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__syntax_error` | | |
| Type mismatch | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__type_mismatch` | | |
| Undeclared identifiers | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__undeclared` | | |
| Redeclarations | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__redeclaration` | | |
| Incompatible pointer types | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__incompatible_ptr` | | |
| Illegal casts | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__illegal_cast` | | |
| Invalid storage class usage | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__invalid_storage` | | |
| Invalid initializers | `diagnostics-recovery` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `12__diag__invalid_initializer` | | |

Diagnostics acceptance rule:

- Verify category and line mapping.
- Do not require exact message string matches.

## Phase 10 — Edge and Torture

Primary buckets:

- `torture-differential`
- `runtime`
- `parser`

Primary oracles:

- `runtime`
- `diag`
- `differential`

| Feature | Bucket | Valid | Negative | Edge | Status | Oracle | Planned Tests | Failures Seen | Notes |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Extremely long expressions | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `diag` | `15__torture__long_expr` | | Stress path |
| Deep nesting | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `diag` | `15__torture__deep_nesting` | | Stress path |
| Many declarations in one file | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `diag` | `15__torture__many_decls` | | |
| Large struct definitions | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `runtime`, `diag` | `15__torture__large_struct` | | ABI-sensitive |
| Pathological declarators | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `ast`, `diag` | `15__torture__pathological_decl` | | |
| Fuzz-compatible harness design | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `runtime` | `15__torture__fuzz_harness` | | Infrastructure item |
| Malformed input robustness | `torture-differential` | [ ] | [ ] | [ ] | `unstarted` | `diag` | `15__torture__malformed_no_crash` | | Must never crash compiler |

## Phase 11 — Implementation-Defined and UB Tracking

This phase is not a separate language feature bucket. It is a tagging layer that
must be applied across the suite.

Every applicable test should be marked when it:

- Uses undefined behavior
- Relies on implementation-defined signedness
- Depends on padding or ABI layout details
- Depends on floating formatting or platform-specific libc behavior

Tracking table:

| Tag Type | Required | Notes |
| --- | --- | --- |
| `ub: true` | Yes for undefined behavior | Exclude from strict differential runtime checks |
| `impl_defined: true` | Yes where language permits platform variance | Keep diagnostics explicit |
| `abi_sensitive: true` | Yes for layout or calling convention assumptions | Avoid brittle cross-target comparisons |
| `float_tolerance: true` | When output may need tolerant comparison | Prefer structured compare over raw strings |

## Bucket Run Log

Use this section as the current campaign tracker. Add dated entries as buckets
are worked.

Suggested entry format:

```md
### 2026-03-02 — Bucket: lexer

- Scope: identifiers, keywords, integer literals
- Added tests: `02__identifiers__ascii_valid`, `02__keywords__identifier_reject`
- Passing: 12
- Failing: 4
- Unsupported: 1 (`universal character names`)
- Main failures:
  - integer suffix parsing rejects valid `ULL`
  - invalid octal literal accepted silently
- Fix batch status: pending
```

## Completion Rule

A phase is only complete when:

- Every row in that phase has planned tests
- Valid coverage exists
- Negative coverage exists where applicable
- Edge coverage exists
- Unsupported features are explicitly marked
- Current failures for the bucket are resolved or intentionally tracked
- The full bucket passes under the current harness target
