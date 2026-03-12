# Compiler Declarations Bucket Report

## Scope

Phase 3 declarations/types bucket (`tests/final/meta/04-declarations.json`).

## Current Baseline

- Active declarations tests: `93`
- Parser bucket (`04-declarations`) status: `93` pass / `0` fail
- Total `tests/final` definitions: `494` tests

## Added In Wave 1

- Storage positives:
  `04__storage__static`, `04__storage__extern`,
  `04__storage__auto`, `04__storage__register`
- Storage fail-closed parse negative:
  `04__storage__missing_declarator`
- Qualifier positives:
  `04__qualifiers__const`, `04__qualifiers__volatile`,
  `04__qualifiers__restrict`
- Qualifier negatives:
  `04__qualifiers__const_pointer_assign`,
  `04__qualifiers__missing_type`

## Added In Storage/Qualifier Fix Pass

- Storage legality negatives:
  `04__storage__file_scope_auto_reject`,
  `04__storage__file_scope_register_reject`
- Storage conflict negatives:
  `04__storage__conflict_static_auto_reject`,
  `04__storage__conflict_extern_register_reject`
- Typedef storage-negative:
  `04__storage__typedef_extern_reject`
- Restrict legality negative:
  `04__qualifiers__restrict_non_pointer_reject`

## Added In Primitive Start

- Integer primitive baseline:
  `04__primitive__integers`
- Floating primitive baseline:
  `04__primitive__floating`
- Bool primitive baseline:
  `04__primitive__bool`

## Added In Primitive Specifier Fix Pass

- Integer modifier conflict negatives:
  `04__primitive__signed_unsigned_conflict_reject`,
  `04__primitive__short_long_conflict_reject`
- Floating modifier legality negative:
  `04__primitive__unsigned_float_reject`
- `_Bool` modifier legality negative:
  `04__primitive__signed_bool_reject`
- `void` modifier legality negative:
  `04__primitive__unsigned_void_reject`

## Added In Char/Enum Follow-up

- Char baseline + negative:
  `04__primitive__char_signedness`,
  `04__primitive__long_char_reject`
- Enum duplicate-negative:
  `04__enum__duplicate`

## Added In Aggregate Negative Hardening

- Struct flexible-array placement negative:
  `04__struct__flex_not_last_reject`
- Union flexible-array legality negative:
  `04__union__flex_array_reject`
- Struct/union VLA-field legality negative:
  `04__struct__vla_field_reject`

## Added In Duplicate/Enum Expansion

- Aggregate duplicate-member negatives:
  `04__struct__duplicate_member_reject`,
  `04__union__duplicate_member_reject`
- Enum value-shape positives:
  `04__enum__explicit_values`,
  `04__enum__implicit_increment`,
  `04__enum__negative_values`

## Added In Parser Shape Expansion

- Struct positives:
  `04__struct__basic`, `04__struct__nested`,
  `04__struct__anonymous`, `04__struct__self_ref`
- Union positives:
  `04__union__basic`, `04__union__nested`, `04__union__anonymous`
- Enum large-value positive:
  `04__enum__large_values`

## Added In Typedef/Bitfield Hardening

- Typedef behavior coverage:
  `04__typedef__redefinition_reject`,
  `04__typedef__shadowing_block`
- Enum constant-expression negative:
  `04__enum__non_integer_value_reject`
- Enum range negative:
  `04__enum__out_of_range_reject`
- Bitfield legality negatives:
  `04__bitfield__negative_width_reject`,
  `04__bitfield__width_exceeds_type_reject`,
  `04__bitfield__non_integral_type_reject`

## Added In Declarator/Enum/Bitfield Expansion

- Declarator complexity positives:
  `04__declarator__ptr_to_fn`,
  `04__declarator__fn_returns_ptr`,
  `04__declarator__ptr_to_array`,
  `04__declarator__array_of_ptrs`,
  `04__declarator__fn_ptr_param`,
  `04__declarator__abstract_fnptr_param`,
  `04__declarator__abstract_ptr_to_array_param`,
  `04__declarator__multi_array`
- Declarator shape negatives:
  `04__declarator__fn_returns_fn_reject`,
  `04__declarator__array_of_fn_reject`,
  `04__declarator__fn_returns_array_reject`
- Enum boundary and overflow-expression coverage:
  `04__enum__implicit_from_min`,
  `04__enum__implicit_overflow_reject`,
  `04__enum__overflow_expr_literal_reject`
- Additional bitfield edge forms:
  `04__bitfield__zero_width_unnamed`,
  `04__bitfield__zero_width_named_reject`,
  `04__bitfield__typedef_integral`,
  `04__bitfield__enum_type`,
  `04__bitfield__unnamed`

## Closed Gaps

- File-scope `auto` and `register` now fail closed.
- Multiple storage-class specifier conflicts now fail closed.
- `typedef` + non-`typedef` storage-class combinations now fail closed.
- Non-pointer `restrict` usage now fails closed.
- Non-final flexible array members in structs now fail closed.
- Flexible array members in unions now fail closed.
- VLA fields inside struct/union declarations now fail closed.
- Duplicate struct/union member names now fail closed.
- Enum explicit, implicit-increment, and negative-valued members now have
  dedicated passing coverage.
- Baseline struct/union basic+nested+anonymous shapes now have dedicated
  passing parser coverage.
- Large enum literals now have dedicated parser/semantic baseline coverage.
- Typedef shadowing/redefinition behavior now has dedicated coverage.
- Bitfield width/type rejection paths now fail closed with dedicated negatives.
- Enumerator non-integer constant-expression rejection now fails closed.
- Enumerator out-of-range and overflow-literal cases now fail closed.
- Declarator complexity matrix now has baseline coverage plus invalid shape
  parse negatives.
- Bitfield zero-width, unnamed, typedef-base, and enum-base forms now have
  dedicated coverage.

## Added In Wave 3

- Enum trailing-comma grammar baseline:
  `04__enum__trailing_comma`

## Added In Wave 4

- Enum empty-body grammar negative:
  `04__enum__empty_reject`

## Added In Wave 5

- Struct tag same-scope redefinition negative:
  `04__struct__tag_redefinition_reject`

## Added In Parserdiag Lane Start

- Promoted parser-shape negatives into standardized lane manifest
  `tests/final/meta/04-declarations-parserdiag.json`:
  `04__parserdiag__struct_member_missing_type_reject`,
  `04__parserdiag__bitfield_missing_colon_reject`,
  `04__parserdiag__enum_missing_rbrace_reject`,
  `04__parserdiag__typedef_missing_identifier_reject`,
  `04__parserdiag__declarator_unbalanced_parens_reject`.

## Added In Runtime/Semantic Follow-up

- Added lane manifest:
  `tests/final/meta/04-declarations-runtime-semantic.json`.
- Promoted runtime overlap anchor:
  `04__union__overlap`.
- Promoted complex legality negatives:
  `04__primitive__complex_int_reject`,
  `04__primitive__complex_unsigned_reject`,
  `04__primitive__complex_missing_base_reject`.

## Probe Status

- Declaration-related runtime and diagnostics probes are currently resolved.
- `tests/final/probes/runtime/04__probe_tag_block_shadow_ok.c` now matches clang.
- `tests/final/probes/runtime/04__probe_fnptr_array_call_runtime.c` now matches clang.
- Parameter constraint probes and block-scope `extern` initializer probe now emit diagnostics as expected.

## Next Targets

1. Continue expanding declaration stress coverage (qualified deep declarators and mixed abstract declarator forms).
2. Promote stable high-value declaration probes into final-suite cases where they are not already represented.
