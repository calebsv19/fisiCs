# Probe Promotion Audit

## Summary

- Total resolved probe inventory audited: `2359`
- Promoted stable coverage: `2145`
- Intentional probe-only coverage: `214`
- Missing promotion candidates: `0`
- Stable final tests scanned: `3744`

### Promoted Match Evidence

- `id`: `986`
- `path`: `1137`
- `stem`: `22`

### Explicit Probe-Only Reasons

- `8`: explicit probe-only family marker: corpus
- `6`: explicit probe-only family marker: pathological
- `10`: explicit probe-only family marker: seeded
- `1`: explicit probe-only note prefix: axis3 wave19 reduced
- `7`: explicit probe-only note prefix: control lane
- `8`: explicit probe-only note prefix: current threshold
- `8`: explicit probe-only note prefix: frontier lane
- `56`: explicit probe-only note prefix: reduced threshold
- `4`: explicit probe-only note prefix: regression guard
- `102`: explicit probe-only note prefix: strict frontier
- `4`: explicit probe-only note prefix: text parity guard

### Bucket Breakdown

- Bucket `01`: promoted `19`, probe-only `0`, missing `0`
- Bucket `02`: promoted `45`, probe-only `0`, missing `0`
- Bucket `03`: promoted `41`, probe-only `0`, missing `0`
- Bucket `04`: promoted `85`, probe-only `2`, missing `0`
- Bucket `05`: promoted `58`, probe-only `8`, missing `0`
- Bucket `06`: promoted `12`, probe-only `44`, missing `0`
- Bucket `07`: promoted `210`, probe-only `0`, missing `0`
- Bucket `08`: promoted `124`, probe-only `40`, missing `0`
- Bucket `09`: promoted `219`, probe-only `20`, missing `0`
- Bucket `10`: promoted `50`, probe-only `33`, missing `0`
- Bucket `11`: promoted `59`, probe-only `42`, missing `0`
- Bucket `12`: promoted `123`, probe-only `0`, missing `0`
- Bucket `13`: promoted `109`, probe-only `0`, missing `0`
- Bucket `14`: promoted `516`, probe-only `1`, missing `0`
- Bucket `15`: promoted `475`, probe-only `24`, missing `0`

## Missing Promotion Candidates

- None.

## Intentional Probe-Only Coverage


### explicit probe-only note prefix: reduced threshold

- `04__probe_deep_declarator_typedef_factory_assignment_runtime` (`runtime`) from `probes/runtime/04__probe_deep_declarator_typedef_factory_assignment_runtime.c` - reduced threshold: typedef-aliased factory assignment lane should compile/run and match clang
- `04__probe_deep_declarator_typedef_factory_runtime` (`runtime`) from `probes/runtime/04__probe_deep_declarator_typedef_factory_runtime.c` - reduced threshold: typedef-aliased factory-call function pointer lane should compile/run and match clang
- `05__probe_line_directive_unary_minus_ptr_reduced_location_pass` (`diagnostic`) from `probes/diagnostics/05__probe_line_directive_unary_minus_ptr_reduced_location_pass.c` - reduced threshold: single-line unary-minus-pointer lane preserves remapped #line boundary

### explicit probe-only note prefix: strict frontier

- `05__probe_diagjson_line_directive_alignof_expr_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_alignof_expr_file_presence_reject.c` - strict frontier: diagnostics JSON should include has_file for _Alignof-expression diagnostic under #line remap

### explicit probe-only note prefix: reduced threshold

- `05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_alignof_expr_reduced_location_pass.c` - reduced threshold: single-line _Alignof-expression lane preserves remapped #line boundary in diagnostics JSON

### explicit probe-only note prefix: strict frontier

- `05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_include_alignof_expr_file_presence_reject.c` - strict frontier: include-header _Alignof-expression diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: control lane

- `05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_include_macro_add_rich_presence_strict.c` - control lane: include-header macro-expanded add-void diagnostics JSON carries rich location (line/column/file)

### explicit probe-only note prefix: strict frontier

- `05__probe_diagjson_line_directive_include_shift_width_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_include_shift_width_file_presence_reject.c` - strict frontier: include-header shift-width diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: control lane

- `05__probe_diagjson_line_directive_macro_add_rich_presence_strict` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_macro_add_rich_presence_strict.c` - control lane: macro-expanded add-void diagnostics JSON carries rich location (line/column/file)

### explicit probe-only note prefix: strict frontier

- `05__probe_diagjson_line_directive_shift_width_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/05__probe_diagjson_line_directive_shift_width_file_presence_reject.c` - strict frontier: diagnostics JSON should include has_file for shift-width diagnostic under #line remap
- `06__probe_line_directive_assign_incompatible_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_assign_incompatible_spelling_reject.c` - strict frontier: assignment-incompatible diagnostics should preserve #line virtual spelling filename
- `06__probe_line_directive_assign_qualifier_loss_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_assign_qualifier_loss_spelling_reject.c` - strict frontier: qualifier-loss assignment diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: current threshold

- `06__probe_line_directive_bitfield_address_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_bitfield_address_spelling_reject.c` - current threshold: bitfield address-of diagnostics under #line include spelling payload

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_bitfield_address_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_bitfield_address_spelling_reject.c` - strict frontier: bitfield address-of diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: reduced threshold

- `06__probe_line_directive_compound_assign_const_lvalue_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_compound_assign_const_lvalue_spelling_reject.c` - reduced threshold: const-lvalue compound-assignment diagnostics should emit nonmodifiable-lvalue rejection

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_compound_assign_const_lvalue_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_compound_assign_const_lvalue_spelling_reject.c` - strict frontier: const-lvalue compound-assignment diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: reduced threshold

- `06__probe_line_directive_compound_assign_pointer_plus_pointer_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject.c` - reduced threshold: pointer-plus-pointer compound-assignment diagnostics should emit pointer-arithmetic rejection

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_compound_assign_pointer_plus_pointer_spelling_reject.c` - strict frontier: pointer-plus-pointer compound-assignment diagnostics should preserve #line virtual spelling filename
- `06__probe_line_directive_include_assign_incompatible_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_assign_incompatible_spelling_reject.c` - strict frontier: include-header assignment-incompatible diagnostics should preserve #line virtual spelling filename
- `06__probe_line_directive_include_assign_qualifier_loss_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_assign_qualifier_loss_spelling_reject.c` - strict frontier: include-header qualifier-loss assignment diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: current threshold

- `06__probe_line_directive_include_bitfield_address_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.c` - current threshold: include-header bitfield address-of diagnostics under #line include spelling payload

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_include_bitfield_address_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_bitfield_address_spelling_reject.c` - strict frontier: include-header bitfield address-of diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: reduced threshold

- `06__probe_line_directive_include_compound_assign_const_lvalue_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.c` - reduced threshold: include-header const-lvalue compound-assignment diagnostics should emit nonmodifiable-lvalue rejection

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_compound_assign_const_lvalue_spelling_reject.c` - strict frontier: include-header const-lvalue compound-assignment diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: reduced threshold

- `06__probe_line_directive_include_compound_assign_pointer_plus_pointer_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.c` - reduced threshold: include-header pointer-plus-pointer compound-assignment diagnostics should emit pointer-arithmetic rejection

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_compound_assign_pointer_plus_pointer_spelling_reject.c` - strict frontier: include-header pointer-plus-pointer compound-assignment diagnostics should preserve #line virtual spelling filename
- `06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_nonmodifiable_lvalue_spelling_reject.c` - strict frontier: include-header nonmodifiable-lvalue assignment diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: current threshold

- `06__probe_line_directive_include_temp_increment_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.c` - current threshold: include-header temporary increment diagnostics under #line include spelling payload

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_include_temp_increment_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_include_temp_increment_spelling_reject.c` - strict frontier: include-header temporary increment diagnostics should preserve #line virtual spelling filename
- `06__probe_line_directive_nonmodifiable_lvalue_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_nonmodifiable_lvalue_spelling_reject.c` - strict frontier: nonmodifiable-lvalue assignment diagnostics should preserve #line virtual spelling filename

### explicit probe-only note prefix: current threshold

- `06__probe_line_directive_temp_increment_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_temp_increment_spelling_reject.c` - current threshold: temporary increment diagnostics under #line include spelling payload

### explicit probe-only note prefix: strict frontier

- `06__probe_line_directive_temp_increment_spelling_reject` (`diagnostic`) from `probes/diagnostics/06__probe_line_directive_temp_increment_spelling_reject.c` - strict frontier: temporary increment diagnostics should preserve #line virtual spelling filename
- `06__probe_diagjson_line_directive_assign_incompatible_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_assign_incompatible_file_presence_reject.c` - strict frontier: assignment-incompatible diagnostics JSON should include has_file under #line remap
- `06__probe_diagjson_line_directive_assign_qualifier_loss_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_assign_qualifier_loss_file_presence_reject.c` - strict frontier: qualifier-loss assignment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: current threshold

- `06__probe_diagjson_line_directive_bitfield_address_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_bitfield_address_file_presence_reject.c` - current threshold: bitfield address-of diagnostics JSON under #line remap includes file presence

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_bitfield_address_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_bitfield_address_file_presence_reject.c` - strict frontier: bitfield address-of diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: reduced threshold

- `06__probe_diagjson_line_directive_compound_assign_const_lvalue_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_compound_assign_const_lvalue_file_presence_reject.c` - reduced threshold: const-lvalue compound-assignment diagnostics JSON under #line remap emits diagnostic payload

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_compound_assign_const_lvalue_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_compound_assign_const_lvalue_file_presence_reject.c` - strict frontier: const-lvalue compound-assignment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: reduced threshold

- `06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject.c` - reduced threshold: pointer-plus-pointer compound-assignment diagnostics JSON under #line remap emits diagnostic payload

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_compound_assign_pointer_plus_pointer_file_presence_reject.c` - strict frontier: pointer-plus-pointer compound-assignment diagnostics JSON should include has_file under #line remap
- `06__probe_diagjson_line_directive_include_assign_incompatible_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_assign_incompatible_file_presence_reject.c` - strict frontier: include-header assignment-incompatible diagnostics JSON should include has_file under #line remap
- `06__probe_diagjson_line_directive_include_assign_qualifier_loss_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_assign_qualifier_loss_file_presence_reject.c` - strict frontier: include-header qualifier-loss assignment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: current threshold

- `06__probe_diagjson_line_directive_include_bitfield_address_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.c` - current threshold: include-header bitfield address-of diagnostics JSON under #line remap includes file presence

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_bitfield_address_file_presence_reject.c` - strict frontier: include-header bitfield address-of diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: reduced threshold

- `06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.c` - reduced threshold: include-header const-lvalue compound-assignment diagnostics JSON under #line remap emits diagnostic payload

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_compound_assign_const_lvalue_file_presence_reject.c` - strict frontier: include-header const-lvalue compound-assignment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: reduced threshold

- `06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.c` - reduced threshold: include-header pointer-plus-pointer compound-assignment diagnostics JSON under #line remap emits diagnostic payload

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_compound_assign_pointer_plus_pointer_file_presence_reject.c` - strict frontier: include-header pointer-plus-pointer compound-assignment diagnostics JSON should include has_file under #line remap
- `06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_nonmodifiable_lvalue_file_presence_reject.c` - strict frontier: include-header nonmodifiable-lvalue assignment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: current threshold

- `06__probe_diagjson_line_directive_include_temp_increment_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.c` - current threshold: include-header temporary increment diagnostics JSON under #line remap includes file presence

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_include_temp_increment_file_presence_reject.c` - strict frontier: include-header temporary increment diagnostics JSON should include has_file under #line remap
- `06__probe_diagjson_line_directive_nonmodifiable_lvalue_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_nonmodifiable_lvalue_file_presence_reject.c` - strict frontier: nonmodifiable-lvalue assignment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: current threshold

- `06__probe_diagjson_line_directive_temp_increment_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_temp_increment_file_presence_reject.c` - current threshold: temporary increment diagnostics JSON under #line remap includes file presence

### explicit probe-only note prefix: strict frontier

- `06__probe_diagjson_line_directive_temp_increment_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/06__probe_diagjson_line_directive_temp_increment_file_presence_reject.c` - strict frontier: temporary increment diagnostics JSON should include has_file under #line remap

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_designator_array_index_negative_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_designator_array_index_negative_spelling_strict.c` - reduced threshold: negative designator-index text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_designator_array_index_negative_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_designator_array_index_negative_spelling_strict.c` - strict frontier: negative designator-index text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_designator_array_index_nonconst_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_designator_array_index_nonconst_spelling_strict.c` - reduced threshold: non-const designator-index text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_designator_array_index_nonconst_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_designator_array_index_nonconst_spelling_strict.c` - strict frontier: non-const designator-index text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_flex_not_last_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_flex_not_last_spelling_strict.c` - reduced threshold: flex-not-last text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_flex_not_last_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_flex_not_last_spelling_strict.c` - strict frontier: flex-not-last text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_include_designator_array_index_negative_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.c` - reduced threshold: include-header negative designator-index text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_include_designator_array_index_negative_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_designator_array_index_negative_spelling_strict.c` - strict frontier: include-header negative designator-index text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_include_designator_array_index_nonconst_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.c` - reduced threshold: include-header non-const designator-index text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_designator_array_index_nonconst_spelling_strict.c` - strict frontier: include-header non-const designator-index text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_include_flex_not_last_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.c` - reduced threshold: include-header flex-not-last text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_include_flex_not_last_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_flex_not_last_spelling_strict.c` - strict frontier: include-header flex-not-last text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_include_union_flex_member_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.c` - reduced threshold: include-header union flexible-member text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_include_union_flex_member_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_include_union_flex_member_spelling_strict.c` - strict frontier: include-header union flexible-member text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diag_line_directive_union_flex_member_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_union_flex_member_spelling_strict.c` - reduced threshold: union flexible-member text diagnostics should emit semantic rejection message

### explicit probe-only note prefix: strict frontier

- `08__probe_diag_line_directive_union_flex_member_spelling_reject` (`diagnostic`) from `probes/diagnostics/08__probe_diag_line_directive_union_flex_member_spelling_strict.c` - strict frontier: union flexible-member text diagnostics under #line remap should preserve spelling file/line

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_designator_array_index_negative_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_designator_array_index_negative_rich_presence.c` - reduced threshold: negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_designator_array_index_negative_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_designator_array_index_negative_rich_presence.c` - strict frontier: negative designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_designator_array_index_nonconst_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_designator_array_index_nonconst_rich_presence.c` - reduced threshold: non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_designator_array_index_nonconst_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_designator_array_index_nonconst_rich_presence.c` - strict frontier: non-const designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_flex_not_last_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_flex_not_last_rich_presence.c` - reduced threshold: flex-not-last diagnostics JSON currently emits sparse semantic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_flex_not_last_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_flex_not_last_rich_presence.c` - strict frontier: flex-not-last diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_include_designator_array_index_negative_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.c` - reduced threshold: include-header negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_include_designator_array_index_negative_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_negative_rich_presence.c` - strict frontier: include-header negative designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_include_designator_array_index_nonconst_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.c` - reduced threshold: include-header non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_include_designator_array_index_nonconst_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_designator_array_index_nonconst_rich_presence.c` - strict frontier: include-header non-const designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_include_flex_not_last_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.c` - reduced threshold: include-header flex-not-last diagnostics JSON currently emits sparse semantic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_include_flex_not_last_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_flex_not_last_rich_presence.c` - strict frontier: include-header flex-not-last diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.c` - reduced threshold: include-header nested negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_negative_rich_presence.c` - strict frontier: include-header nested negative designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.c` - reduced threshold: include-header nested non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_nested_designator_array_index_nonconst_rich_presence.c` - strict frontier: include-header nested non-const designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_include_union_flex_member_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.c` - reduced threshold: include-header union flexible-member diagnostics JSON currently emits sparse semantic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_include_union_flex_member_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_include_union_flex_member_rich_presence.c` - strict frontier: include-header union flexible-member diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_nested_designator_array_index_negative_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_negative_rich_presence.c` - reduced threshold: nested negative designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_nested_designator_array_index_negative_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_negative_rich_presence.c` - strict frontier: nested negative designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_rich_presence.c` - reduced threshold: nested non-const designator-index diagnostics JSON under #line remap emits initializer diagnostic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_nested_designator_array_index_nonconst_rich_presence.c` - strict frontier: nested non-const designator-index diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `08__probe_diagjson_line_directive_union_flex_member_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_union_flex_member_rich_presence.c` - reduced threshold: union flexible-member diagnostics JSON currently emits sparse semantic payload

### explicit probe-only note prefix: strict frontier

- `08__probe_diagjson_line_directive_union_flex_member_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/08__probe_diagjson_line_directive_union_flex_member_rich_presence.c` - strict frontier: union flexible-member diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `09__probe_diag_line_directive_include_switch_double_condition_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.c` - reduced threshold: include-header #line double switch-condition text diagnostics should emit control-type rejection

### explicit probe-only note prefix: strict frontier

- `09__probe_diag_line_directive_include_switch_double_condition_spelling_reject` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_include_switch_double_condition_spelling_strict.c` - strict frontier: include-header #line double switch-condition text diagnostics should preserve remapped line

### explicit probe-only note prefix: reduced threshold

- `09__probe_diag_line_directive_include_switch_pointer_condition_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.c` - reduced threshold: include-header #line pointer switch-condition text diagnostics should emit control-type rejection

### explicit probe-only note prefix: strict frontier

- `09__probe_diag_line_directive_include_switch_pointer_condition_spelling_reject` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_include_switch_pointer_condition_spelling_strict.c` - strict frontier: include-header #line pointer switch-condition text diagnostics should preserve remapped line

### explicit probe-only note prefix: reduced threshold

- `09__probe_diag_line_directive_include_switch_string_condition_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.c` - reduced threshold: include-header #line string-literal switch-condition text diagnostics should emit control-type rejection

### explicit probe-only note prefix: strict frontier

- `09__probe_diag_line_directive_include_switch_string_condition_spelling_reject` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_include_switch_string_condition_spelling_strict.c` - strict frontier: include-header #line string-literal switch-condition text diagnostics should preserve remapped line

### explicit probe-only note prefix: reduced threshold

- `09__probe_diag_line_directive_switch_double_condition_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_switch_double_condition_spelling_strict.c` - reduced threshold: #line double switch-condition text diagnostics should emit control-type rejection

### explicit probe-only note prefix: strict frontier

- `09__probe_diag_line_directive_switch_double_condition_spelling_reject` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_switch_double_condition_spelling_strict.c` - strict frontier: #line double switch-condition text diagnostics should preserve remapped line

### explicit probe-only note prefix: reduced threshold

- `09__probe_diag_line_directive_switch_pointer_condition_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_switch_pointer_condition_spelling_strict.c` - reduced threshold: #line pointer switch-condition text diagnostics should emit control-type rejection

### explicit probe-only note prefix: strict frontier

- `09__probe_diag_line_directive_switch_pointer_condition_spelling_reject` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_switch_pointer_condition_spelling_strict.c` - strict frontier: #line pointer switch-condition text diagnostics should preserve remapped line

### explicit probe-only note prefix: reduced threshold

- `09__probe_diag_line_directive_switch_string_condition_current_sparse_pass` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_switch_string_condition_spelling_strict.c` - reduced threshold: #line string-literal switch-condition text diagnostics should emit control-type rejection

### explicit probe-only note prefix: strict frontier

- `09__probe_diag_line_directive_switch_string_condition_spelling_reject` (`diagnostic`) from `probes/diagnostics/09__probe_diag_line_directive_switch_string_condition_spelling_strict.c` - strict frontier: #line string-literal switch-condition text diagnostics should preserve remapped line

### explicit probe-only note prefix: reduced threshold

- `09__probe_diagjson_line_directive_break_outside_loop_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_break_outside_loop_rich_presence.c` - reduced threshold: break-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload

### explicit probe-only note prefix: strict frontier

- `09__probe_diagjson_line_directive_break_outside_loop_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_break_outside_loop_rich_presence.c` - strict frontier: break-outside-loop diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `09__probe_diagjson_line_directive_continue_outside_loop_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_continue_outside_loop_rich_presence.c` - reduced threshold: continue-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload

### explicit probe-only note prefix: strict frontier

- `09__probe_diagjson_line_directive_continue_outside_loop_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_continue_outside_loop_rich_presence.c` - strict frontier: continue-outside-loop diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `09__probe_diagjson_line_directive_include_break_outside_loop_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_include_break_outside_loop_rich_presence.c` - reduced threshold: include-header break-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload

### explicit probe-only note prefix: strict frontier

- `09__probe_diagjson_line_directive_include_break_outside_loop_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_include_break_outside_loop_rich_presence.c` - strict frontier: include-header break-outside-loop diagnostics JSON under #line remap should include file presence

### explicit probe-only note prefix: reduced threshold

- `09__probe_diagjson_line_directive_include_continue_outside_loop_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_include_continue_outside_loop_rich_presence.c` - reduced threshold: include-header continue-outside-loop diagnostics JSON under #line remap emits semantic diagnostic payload

### explicit probe-only note prefix: strict frontier

- `09__probe_diagjson_line_directive_include_continue_outside_loop_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/09__probe_diagjson_line_directive_include_continue_outside_loop_rich_presence.c` - strict frontier: include-header continue-outside-loop diagnostics JSON under #line remap should include file presence
- `10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict.c` - strict frontier: extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap
- `10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict.c` - strict frontier: extern-array mismatch with first declaration remapped by #line should emit conflict diagnostics
- `10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict.c` - strict frontier: extern-array mismatch with second declaration remapped by #line should emit conflict diagnostics
- `10__probe_diag_line_directive_extern_array_mismatch_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_mismatch_spelling_strict.c` - strict frontier: extern-array mismatch should emit linkage conflict diagnostics under #line remap

### explicit probe-only note prefix: control lane

- `10__probe_diag_line_directive_extern_array_vs_scalar_conflict_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_vs_scalar_conflict_spelling_strict.c` - control lane: extern array-vs-scalar redeclaration should still emit conflict diagnostics under #line

### explicit probe-only note prefix: strict frontier

- `10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict.c` - strict frontier: include extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap
- `10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict.c` - strict frontier: include extern-array mismatch should emit linkage conflict diagnostics under #line remap
- `10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_multitu_extern_array_def_mismatch_spelling_strict_main.c` - strict frontier: multi-TU extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap

### explicit probe-only note prefix: regression guard

- `10__probe_diag_line_directive_multitu_extern_type_conflict_current_linkstage_pass` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_main.c` - regression guard: multi-TU extern type-conflict should keep semantic spelling under #line

### explicit probe-only note prefix: strict frontier

- `10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_multitu_extern_type_conflict_spelling_strict_main.c` - strict frontier: multi-TU extern type-conflict should preserve source spelling under #line
- `10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_multitu_include_extern_array_def_mismatch_spelling_strict_main.c` - strict frontier: multi-TU include extern-array declaration/definition extent mismatch should emit conflict diagnostics under #line remap

### explicit probe-only note prefix: regression guard

- `10__probe_diag_line_directive_multitu_include_extern_type_conflict_current_linkstage_pass` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_main.c` - regression guard: multi-TU include extern type-conflict should keep semantic spelling under #line

### explicit probe-only note prefix: strict frontier

- `10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict` (`diagnostic`) from `probes/diagnostics/10__probe_diag_line_directive_multitu_include_extern_type_conflict_spelling_strict_main.c` - strict frontier: multi-TU include extern type-conflict should preserve source spelling under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_block_extern_different_type_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_block_extern_different_type_rich_strict.c` - frontier lane: block-scope extern type-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: strict frontier

- `10__probe_diagjson_line_directive_extern_array_def_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_def_mismatch_spelling_strict.c` - strict frontier: extern-array declaration/definition mismatch diagnostics JSON should preserve remapped file/line under #line
- `10__probe_diagjson_line_directive_extern_array_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_mismatch_spelling_strict.c` - strict frontier: extern-array mismatch diagnostics JSON should preserve remapped file/line under #line
- `10__probe_diagjson_line_directive_extern_array_mismatch_first_decl_line_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_mismatch_first_decl_line_spelling_strict.c` - strict frontier: extern-array mismatch first-decl placement diagnostics JSON should preserve remapped file/line under #line
- `10__probe_diagjson_line_directive_extern_array_mismatch_second_decl_line_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/10__probe_diag_line_directive_extern_array_mismatch_second_decl_line_spelling_strict.c` - strict frontier: extern-array mismatch second-decl placement diagnostics JSON should preserve remapped file/line under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_extern_static_mismatch_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_extern_static_mismatch_rich_strict.c` - frontier lane: extern/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: control lane

- `10__probe_diagjson_line_directive_extern_type_mismatch_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_extern_type_mismatch_rich_strict.c` - control lane: extern type-mismatch diagnostics JSON should carry remapped line/file/hint under #line
- `10__probe_diagjson_line_directive_function_redecl_conflict_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_function_redecl_conflict_rich_strict.c` - control lane: function redeclaration-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_include_block_extern_different_type_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_include_block_extern_different_type_rich_strict.c` - frontier lane: include-header block-scope extern type-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: strict frontier

- `10__probe_diagjson_line_directive_include_extern_array_def_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/10__probe_diag_line_directive_include_extern_array_def_mismatch_spelling_strict.c` - strict frontier: include extern-array declaration/definition mismatch diagnostics JSON should preserve remapped file/line under #line
- `10__probe_diagjson_line_directive_include_extern_array_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/10__probe_diag_line_directive_include_extern_array_mismatch_spelling_strict.c` - strict frontier: include extern-array mismatch diagnostics JSON should preserve remapped file/line under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_include_extern_static_mismatch_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_include_extern_static_mismatch_rich_strict.c` - frontier lane: include-header extern/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: control lane

- `10__probe_diagjson_line_directive_include_extern_type_mismatch_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_include_extern_type_mismatch_rich_strict.c` - control lane: include-header extern type-mismatch diagnostics JSON should carry remapped line/file/hint under #line
- `10__probe_diagjson_line_directive_include_function_redecl_conflict_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_include_function_redecl_conflict_rich_strict.c` - control lane: include-header function redeclaration-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_include_tentative_static_conflict_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_include_tentative_static_conflict_rich_strict.c` - frontier lane: include-header tentative/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: regression guard

- `10__probe_diagjson_line_directive_multitu_extern_type_conflict_current_linkstage_pass` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_main.c` - regression guard: multi-TU extern type-conflict should keep semantic remapped location richness under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_multitu_extern_type_conflict_rich_strict_main.c` - frontier lane: multi-TU extern type-conflict diagnostics JSON should preserve remapped location richness under #line

### explicit probe-only note prefix: regression guard

- `10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_current_linkstage_pass` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_main.c` - regression guard: multi-TU include extern type-conflict should keep semantic remapped location richness under #line

### explicit probe-only note prefix: frontier lane

- `10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_multitu_include_extern_type_conflict_rich_strict_main.c` - frontier lane: multi-TU include extern type-conflict diagnostics JSON should preserve remapped location richness under #line
- `10__probe_diagjson_line_directive_tentative_static_conflict_rich_strict` (`diagnostic-json`) from `probes/diagnostics/10__probe_diagjson_line_directive_tentative_static_conflict_rich_strict.c` - frontier lane: tentative/static linkage-conflict diagnostics JSON should carry remapped line/file/hint under #line

### explicit probe-only note prefix: text parity guard

- `11__probe_diag_line_directive_multitu_include_parserdiag_decl_missing_rparen_text_threshold_pass` (`diagnostic`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence.c` - text parity guard: multi-TU include #line parser declarator recovery remains stable in text diagnostics
- `11__probe_diag_line_directive_multitu_include_too_many_args_text_threshold_pass` (`diagnostic`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence.c` - text parity guard: multi-TU include #line too-many-args remains stable in text diagnostics
- `11__probe_diag_line_directive_multitu_parserdiag_decl_missing_rparen_text_threshold_pass` (`diagnostic`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence.c` - text parity guard: multi-TU #line parser declarator recovery remains stable in text diagnostics
- `11__probe_diag_line_directive_multitu_too_many_args_text_threshold_pass` (`diagnostic`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence.c` - text parity guard: multi-TU #line too-many-args remains stable in text diagnostics

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_argument_type_mismatch_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_argument_type_mismatch_file_presence.c` - reduced threshold: #line argument-type-mismatch diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_argument_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_argument_type_mismatch_file_presence.c` - strict frontier: #line argument-type-mismatch diagjson should preserve file presence
- `11__probe_diagjson_line_directive_fnptr_assign_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_fnptr_assign_file_presence.c` - strict frontier: #line function-pointer assignment diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_argument_type_mismatch_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_argument_type_mismatch_file_presence.c` - reduced threshold: include #line argument-type-mismatch diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_argument_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_argument_type_mismatch_file_presence.c` - strict frontier: include #line argument-type-mismatch diagjson should preserve file presence
- `11__probe_diagjson_line_directive_include_fnptr_assign_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_fnptr_assign_file_presence.c` - strict frontier: include #line function-pointer assignment diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_nonvoid_missing_return_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_nonvoid_missing_return_file_presence.c` - reduced threshold: include #line non-void-missing-return diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_nonvoid_missing_return_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_nonvoid_missing_return_file_presence.c` - strict frontier: include #line non-void-missing-return diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_return_type_mismatch_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_return_type_mismatch_file_presence.c` - reduced threshold: include #line return-type-mismatch diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_return_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_return_type_mismatch_file_presence.c` - strict frontier: include #line return-type-mismatch diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_too_few_args_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_too_few_args_file_presence.c` - reduced threshold: include #line prototype-too-few-args diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_too_few_args_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_too_few_args_file_presence.c` - strict frontier: include #line prototype-too-few-args diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_too_many_args_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_too_many_args_file_presence.c` - reduced threshold: include #line prototype-too-many-args diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_too_many_args_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_too_many_args_file_presence.c` - strict frontier: include #line prototype-too-many-args diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_typedef_fnptr_argument_type_mismatch_current_empty_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_typedef_fnptr_argument_type_mismatch_file_presence.c` - reduced threshold: include typedef-wrapped function-pointer call arg-mismatch preserves the remapped line in diagjson

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_typedef_fnptr_argument_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_typedef_fnptr_argument_type_mismatch_file_presence.c` - strict frontier: include #line typedef-wrapped function-pointer call arg-mismatch diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_include_void_return_value_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_void_return_value_file_presence.c` - reduced threshold: include #line void-return-value diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_include_void_return_value_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_include_void_return_value_file_presence.c` - strict frontier: include #line void-return-value diagjson should preserve file presence
- `11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_fnptr_assign_file_presence.c` - strict frontier: multi-TU #line function-pointer assignment diagjson should preserve remapped line and file presence
- `11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_include_fnptr_assign_file_presence.c` - strict frontier: multi-TU include #line function-pointer assignment diagjson should preserve remapped line and file presence
- `11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_include_parserdiag_decl_missing_rparen_presence.c` - strict frontier: multi-TU include #line parser-declarator recovery should preserve parser diagjson presence
- `11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_include_return_type_mismatch_file_presence.c` - strict frontier: multi-TU include #line return-type-mismatch diagjson should preserve remapped line and file presence
- `11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_include_too_many_args_file_presence.c` - strict frontier: multi-TU include #line too-many-args diagjson should preserve remapped line and file presence
- `11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_parserdiag_decl_missing_rparen_presence.c` - strict frontier: multi-TU #line parser-declarator recovery should preserve parser diagjson presence
- `11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_return_type_mismatch_file_presence.c` - strict frontier: multi-TU #line return-type-mismatch diagjson should preserve remapped line and file presence
- `11__probe_diagjson_line_directive_multitu_too_many_args_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_multitu_too_many_args_file_presence.c` - strict frontier: multi-TU #line too-many-args diagjson should preserve remapped line and file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_nonvoid_missing_return_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_nonvoid_missing_return_file_presence.c` - reduced threshold: #line non-void-missing-return diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_nonvoid_missing_return_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_nonvoid_missing_return_file_presence.c` - strict frontier: #line non-void-missing-return diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_return_type_mismatch_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_return_type_mismatch_file_presence.c` - reduced threshold: #line return-type-mismatch diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_return_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_return_type_mismatch_file_presence.c` - strict frontier: #line return-type-mismatch diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_too_few_args_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_too_few_args_file_presence.c` - reduced threshold: #line prototype-too-few-args diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_too_few_args_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_too_few_args_file_presence.c` - strict frontier: #line prototype-too-few-args diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_too_many_args_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_too_many_args_file_presence.c` - reduced threshold: #line prototype-too-many-args diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_too_many_args_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_too_many_args_file_presence.c` - strict frontier: #line prototype-too-many-args diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_typedef_fnptr_argument_type_mismatch_current_empty_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_typedef_fnptr_argument_type_mismatch_file_presence.c` - reduced threshold: typedef-wrapped function-pointer call arg-mismatch preserves the remapped line in diagjson

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_typedef_fnptr_argument_type_mismatch_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_typedef_fnptr_argument_type_mismatch_file_presence.c` - strict frontier: #line typedef-wrapped function-pointer call arg-mismatch diagjson should preserve file presence

### explicit probe-only note prefix: reduced threshold

- `11__probe_diagjson_line_directive_void_return_value_current_sparse_pass` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_void_return_value_file_presence.c` - reduced threshold: #line void-return-value diagjson preserves remapped line with sparse location metadata

### explicit probe-only note prefix: strict frontier

- `11__probe_diagjson_line_directive_void_return_value_file_presence_reject` (`diagnostic-json`) from `probes/diagnostics/11__probe_diagjson_line_directive_void_return_value_file_presence.c` - strict frontier: #line void-return-value diagjson should preserve file presence

### explicit probe-only note prefix: axis3 wave19 reduced

- `14__probe_axis3_wave19_multitu_abi_hfa_struct_return_variadic_shadow_replay_fold_edge_reduced` (`runtime`) from `probes/runtime/14__probe_axis3_wave19_multitu_abi_hfa_struct_return_variadic_shadow_replay_fold_edge_reduced_main.c` - axis3 wave19 reduced: smaller shadow/replay HFA variadic fold boundary remains clang-parity stable as the strict lane is now fixed and promoted

### explicit probe-only family marker: corpus

- `15__probe_diag_corpus_external_compile_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_compile_reject.c` - external-corpus fragment with malformed typedef tail should fail closed with parser diagnostic
- `15__probe_diag_corpus_external_include_guard_mismatch_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_include_guard_mismatch_reject.c` - external-corpus include-guard mismatch should fail closed via recursive-include detection
- `15__probe_diag_corpus_external_macro_chain_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_macro_chain_reject.c` - external-corpus malformed macro-chain fragment should fail closed in preprocessing
- `15__probe_diag_corpus_external_macro_guard_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_external_macro_guard_reject.c` - external-corpus malformed macro-guard fragment should fail closed in preprocessing
- `15__probe_diag_corpus_pinned_macro_include_chain_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_pinned_macro_include_chain_reject.c` - pinned corpus macro include-chain fragment should fail closed in preprocessing
- `15__probe_diag_corpus_pinned_typedef_decl_cycle_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_corpus_pinned_typedef_decl_cycle_reject.c` - pinned corpus typedef-decl cycle fragment should fail closed with parser diagnostic

### explicit probe-only family marker: seeded

- `15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash.c` - seeded malformed fuzz-volume replay lane should fail closed without crashing
- `15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash.c` - malformed nested #if/#elif chain should fail closed without crashing
- `15__probe_diag_malformed_token_stream_seeded_a_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_a_no_crash.c` - seeded malformed token-stream A should fail closed without crashing
- `15__probe_diag_malformed_token_stream_seeded_b_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_b_no_crash.c` - seeded malformed token-stream B should fail closed without crashing
- `15__probe_diag_malformed_token_stream_seeded_c_no_crash` (`diagnostic`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_c_no_crash.c` - seeded malformed token-stream C should fail closed without crashing

### explicit probe-only family marker: pathological

- `15__probe_diag_pathological_initializer_rewrite_surface_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_pathological_initializer_rewrite_surface_reject.c` - pathological initializer rewrite surface lane should fail closed with deterministic diagnostics
- `15__probe_diag_pathological_initializer_shape_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_pathological_initializer_shape_reject.c` - pathological designated-initializer shape should fail closed with deterministic diagnostics
- `15__probe_diag_pathological_switch_case_surface_reject` (`diagnostic`) from `probes/diagnostics/15__probe_diag_pathological_switch_case_surface_reject.c` - pathological switch case surface lane should reject non-constant case labels

### explicit probe-only family marker: corpus

- `15__probe_diagjson_corpus_pinned_macro_include_chain_reject` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_corpus_pinned_macro_include_chain_reject.c` - diagnostics JSON should be exported for pinned-corpus macro include-chain rejection
- `15__probe_diagjson_corpus_pinned_typedef_decl_cycle_reject` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_corpus_pinned_typedef_decl_cycle_reject.c` - diagnostics JSON should be exported for pinned-corpus typedef-decl-cycle rejection

### explicit probe-only family marker: seeded

- `15__probe_diagjson_fuzz_seeded_malformed_volume_replay_no_crash` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_fuzz_seeded_malformed_volume_replay_no_crash.c` - diagnostics JSON should be exported for seeded malformed fuzz-volume replay lane
- `15__probe_diagjson_malformed_pp_nested_ifdef_chain_seeded_d_no_crash` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_malformed_pp_nested_ifdef_chain_seeded_d_no_crash.c` - diagnostics JSON should be exported for malformed nested #if/#elif chain lane
- `15__probe_diagjson_malformed_token_stream_seeded_a_no_crash` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_a_no_crash.c` - diagnostics JSON should be exported for seeded malformed token-stream A
- `15__probe_diagjson_malformed_token_stream_seeded_b_no_crash` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_b_no_crash.c` - diagnostics JSON should be exported for seeded malformed token-stream B
- `15__probe_diagjson_malformed_token_stream_seeded_c_no_crash` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_malformed_token_stream_seeded_c_no_crash.c` - diagnostics JSON should be exported for seeded malformed token-stream C

### explicit probe-only family marker: pathological

- `15__probe_diagjson_pathological_initializer_rewrite_surface_reject` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_pathological_initializer_rewrite_surface_reject.c` - diagnostics JSON should be exported for pathological initializer-rewrite surface rejection
- `15__probe_diagjson_pathological_initializer_shape_reject` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_pathological_initializer_shape_reject.c` - diagnostics JSON should be exported for pathological designated-initializer rejection
- `15__probe_diagjson_pathological_switch_case_surface_reject` (`diagnostic-json`) from `probes/diagnostics/15__probe_diag_pathological_switch_case_surface_reject.c` - diagnostics JSON should be exported for pathological non-constant switch-case surface rejection

## Promoted Coverage Sample

- `01__probe_line_directive_include_nested_macro_undeclared_identifier_location_strict` via `id` -> 01__diag__line_directive_include_nested_macro_undeclared_identifier_location_strict, 01__diagjson__line_directive_include_nested_macro_undeclared_identifier_location_strict
- `01__probe_line_directive_include_tokenpaste_undeclared_identifier_location_strict` via `id` -> 01__diag__line_directive_include_tokenpaste_undeclared_identifier_location_strict, 01__diagjson__line_directive_include_tokenpaste_undeclared_identifier_location_strict
- `01__probe_line_directive_macro_nonvoid_return_current_zerozero` via `id` -> 01__diag__line_directive_macro_nonvoid_return_current_zerozero, 01__diagjson__line_directive_macro_nonvoid_return_current_zerozero
- `01__probe_line_directive_macro_nonvoid_return_location_reject` via `id` -> 01__diag__line_directive_macro_nonvoid_return_location_reject, 01__diagjson__line_directive_macro_nonvoid_return_location_reject
- `01__probe_line_directive_virtual_line_nonvoid_return_current_zerozero` via `id` -> 01__diag__line_directive_virtual_line_nonvoid_return_current_zerozero
- `01__probe_line_directive_virtual_line_nonvoid_return_location_reject` via `id` -> 01__diag__line_directive_virtual_line_nonvoid_return_location_reject, 01__diagjson__line_directive_nonvoid_return_location_reject
- `01__probe_line_directive_virtual_line_spelling_reject` via `id` -> 01__diag__line_directive_virtual_line_spelling_reject
- `01__probe_line_directive_virtual_line_undeclared_identifier_location_reject` via `id` -> 01__diag__line_directive_virtual_line_undeclared_identifier_location_reject
- `01__probe_line_directive_virtual_macro_filename_spelling_reject` via `id` -> 01__diag__line_directive_virtual_macro_filename_spelling_reject
- `01__probe_line_mapping_real_file_baseline` via `id` -> 01__diag__line_mapping_real_file_baseline
- `01__probe_nonvoid_return_plain_current_zerozero` via `id` -> 01__diag__nonvoid_return_plain_current_zerozero, 01__diagjson__nonvoid_return_plain_current_zerozero
- `01__probe_diagjson_line_directive_include_nested_macro_undeclared_identifier_location_strict` via `path` -> 01__diag__line_directive_include_nested_macro_undeclared_identifier_location_strict, 01__diagjson__line_directive_include_nested_macro_undeclared_identifier_location_strict
- `01__probe_diagjson_line_directive_include_tokenpaste_undeclared_identifier_location_strict` via `path` -> 01__diag__line_directive_include_tokenpaste_undeclared_identifier_location_strict, 01__diagjson__line_directive_include_tokenpaste_undeclared_identifier_location_strict
- `01__probe_diagjson_line_directive_macro_line_map_strict` via `path` -> 01__diagjson__line_directive_macro_line_map_strict
- `01__probe_diagjson_line_directive_macro_nonvoid_return_current_zerozero` via `path` -> 01__diag__line_directive_macro_nonvoid_return_current_zerozero, 01__diagjson__line_directive_macro_nonvoid_return_current_zerozero
- `01__probe_diagjson_line_directive_macro_nonvoid_return_location_reject` via `path` -> 01__diag__line_directive_macro_nonvoid_return_location_reject, 01__diagjson__line_directive_macro_nonvoid_return_location_reject
- `01__probe_diagjson_line_directive_nonvoid_return_location_reject` via `path` -> 01__diag__line_directive_virtual_line_nonvoid_return_location_reject, 01__diagjson__line_directive_nonvoid_return_location_reject
- `01__probe_diagjson_line_directive_undeclared_identifier_location_strict` via `path` -> 01__diagjson__line_directive_undeclared_identifier_location_strict
- `01__probe_diagjson_nonvoid_return_plain_current_zerozero` via `path` -> 01__diag__nonvoid_return_plain_current_zerozero, 01__diagjson__nonvoid_return_plain_current_zerozero
- `02__probe_lexer_line_directive_char_invalid_hex_escape_current_physical_line` via `id` -> 02__diag__lexer_line_directive_char_invalid_hex_escape_current_physical_line, 02__diagjson__lexer_line_directive_char_invalid_hex_escape_current_physical_line
- `02__probe_lexer_line_directive_char_invalid_hex_escape_location_reject` via `id` -> 02__diag__lexer_line_directive_char_invalid_hex_escape_location_reject, 02__diagjson__lexer_line_directive_char_invalid_hex_escape_location_reject
- `02__probe_lexer_line_directive_include_char_invalid_hex_escape_current_physical_line` via `path` -> 02__line_directive_include_char_invalid_hex_escape_diagjson_current_physical_line
- `02__probe_lexer_line_directive_include_invalid_at_current_physical_line` via `path` -> 02__line_directive_include_invalid_at_diagjson_current_physical_line
- `02__probe_lexer_line_directive_include_invalid_backtick_current_physical_line` via `path` -> 02__line_directive_include_invalid_backtick_diagjson_current_physical_line
- `02__probe_lexer_line_directive_include_invalid_dollar_current_physical_line` via `path` -> 02__line_directive_include_invalid_dollar_diagjson_current_physical_line
- `02__probe_lexer_line_directive_include_string_invalid_escape_current_physical_line` via `path` -> 02__line_directive_include_string_invalid_escape_diagjson_current_physical_line
- `02__probe_lexer_line_directive_include_ucn_identifier_unsupported_current_physical_line` via `path` -> 02__line_directive_include_ucn_identifier_unsupported_diagjson_current_parser_only
- `02__probe_lexer_line_directive_include_unterminated_char_current_physical_line` via `path` -> 02__line_directive_include_unterminated_char_diagjson_current_physical_line
- `02__probe_lexer_line_directive_include_unterminated_string_current_physical_line` via `path` -> 02__line_directive_include_unterminated_string_diagjson_current_physical_line
- `02__probe_lexer_line_directive_invalid_at_current_physical_line` via `id` -> 02__diag__lexer_line_directive_invalid_at_current_physical_line, 02__diagjson__lexer_line_directive_invalid_at_current_physical_line
- `02__probe_lexer_line_directive_invalid_at_location_reject` via `id` -> 02__diag__lexer_line_directive_invalid_at_location_reject, 02__diagjson__lexer_line_directive_invalid_at_location_reject
- `02__probe_lexer_line_directive_invalid_backtick_current_physical_line` via `id` -> 02__diag__lexer_line_directive_invalid_backtick_current_physical_line, 02__diagjson__lexer_line_directive_invalid_backtick_current_physical_line
- `02__probe_lexer_line_directive_invalid_backtick_location_reject` via `id` -> 02__diag__lexer_line_directive_invalid_backtick_location_reject, 02__diagjson__lexer_line_directive_invalid_backtick_location_reject
- `02__probe_lexer_line_directive_invalid_dollar_current_physical_line` via `id` -> 02__diag__lexer_line_directive_invalid_dollar_current_physical_line, 02__diagjson__lexer_line_directive_invalid_dollar_current_physical_line
- `02__probe_lexer_line_directive_invalid_dollar_location_reject` via `id` -> 02__diag__lexer_line_directive_invalid_dollar_location_reject, 02__diagjson__lexer_line_directive_invalid_dollar_location_reject
- `02__probe_lexer_line_directive_string_invalid_escape_current_physical_line` via `id` -> 02__diag__lexer_line_directive_string_invalid_escape_current_physical_line, 02__diagjson__lexer_line_directive_string_invalid_escape_current_physical_line
- `02__probe_lexer_line_directive_string_invalid_escape_location_reject` via `id` -> 02__diag__lexer_line_directive_string_invalid_escape_location_reject, 02__diagjson__lexer_line_directive_string_invalid_escape_location_reject
- `02__probe_lexer_line_directive_ucn_identifier_unsupported_current_physical_line` via `path` -> 02__line_directive_ucn_identifier_unsupported_diagjson_current_empty
- `02__probe_lexer_line_directive_unterminated_char_current_physical_line` via `id` -> 02__diag__lexer_line_directive_unterminated_char_current_physical_line, 02__diagjson__lexer_line_directive_unterminated_char_current_physical_line
- `02__probe_lexer_line_directive_unterminated_char_location_reject` via `id` -> 02__diag__lexer_line_directive_unterminated_char_location_reject, 02__diagjson__lexer_line_directive_unterminated_char_location_reject
