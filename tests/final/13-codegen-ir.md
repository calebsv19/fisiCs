# Codegen / IR Correctness

## Scope
Lowering semantics once IR/codegen is enabled.

## Validate
- Correct addressing for locals/globals.
- Struct layout/offsets and short-circuit lowering.
- Ternary and switch lowering.
- Volatile loads/stores emit `load volatile`/`store volatile` in IR.

## Acceptance Checklist
- IR emitted for basic arithmetic and control flow.
- Short-circuit logic lowers correctly.
- Ternary expression lowers to conditional branches.
- Switch lowering handles dense/sparse cases.

## Test Cases (initial list)
1) `13__ir_basic_arith`
   - Simple arithmetic and return.
2) `13__ir_short_circuit`
   - && and || lowering.
3) `13__ir_ternary`
   - Conditional operator lowering.
4) `13__ir_switch`
   - switch lowering for multiple cases.
5) `13__ir_volatile`
   - volatile load/store lowering.
6) `13__ir_ptr_arith`
   - pointer arithmetic lowering.
7) `13__ir_struct_access`
   - struct field access lowering.
8) `13__ir_logical_not`
   - logical not lowering.
9) `13__ir_bitwise`
   - bitwise ops lowering.
10) `13__ir_short_circuit_side_effect`
   - short-circuit with side effects.
11) `13__ir_compare_branch`
   - comparison lowering to boolean.
12) `13__ir_global_init`
   - global initializer emission.
13) `13__ir_nested_loops`
   - nested loop control-flow lowering.
14) `13__ir_function_pointer_call`
   - function pointer call lowering.

## Expected Outputs
- IR expectations (`.ir`) for codegen output.

## Wave 0 Baseline Snapshot
- Active-suite targeted sweep for `13__*` and `14__*`:
  - selected: `33`
  - pass: `33`
  - fail: `0`
  - skip: `0`

## Wave 1 Additions (Probe Expansion)
1) `13__probe_phi_continue_runtime`
   - Loop-carried accumulator with `continue`/`break` edges.
2) `13__probe_short_circuit_chain_runtime`
   - Nested short-circuit side-effect ordering.
3) `13__probe_struct_copy_runtime`
   - Struct return + by-value copy update runtime path.
4) `13__probe_ptr_stride_runtime`
   - Pointer-difference scaling and non-`int` element stride.
5) `13__probe_global_init_runtime`
   - Partial global initializer and zero-fill runtime behavior.

## Probe Status
- `PROBE_FILTER=13__probe_ python3 tests/final/probes/run_probes.py`
  - blocked: `0`
  - resolved: `5`
  - skipped: `0`
- `13__probe_ptr_stride_runtime` now resolves after pointer element-width
  scaling fix in codegen pointer arithmetic/difference lowering.

## Wave 2 Promotion
- Resolved Wave 1 runtime probes are promoted into active runtime-surface wave 2:
  - `14__runtime_loop_continue_break_phi`
  - `14__runtime_short_circuit_chain_effects`
  - `14__runtime_struct_copy_update`
  - `14__runtime_pointer_stride_long_long`
  - `14__runtime_global_partial_init_zerofill`

## Wave 3 Additions (IR Hardening)
- Added active `13` IR tests:
  - `13__ir_ptrdiff_long_long`
  - `13__ir_ptr_add_scaled_i64`
  - `13__ir_switch_loop_continue`
  - `13__ir_do_while`
- Coverage intent:
  - pointer difference and scaled pointer-add lowering on `long long*`
  - `switch` + loop `continue` edge correctness in IR CFG
  - do-while control-flow lowering shape
- Current `13__*` targeted sweep: `18/18` passing.

## Wave 4 Additions (Stringent CFG + Pointer Stress)
- Added active `13` IR tests:
  - `13__ir_nested_loop_break_continue`
  - `13__ir_switch_fallthrough_sparse`
  - `13__ir_do_while_continue_break`
  - `13__ir_ternary_logical_chain`
  - `13__ir_ptr_stride_mixed`
- Coverage intent:
  - nested loop edge lowering with both `break` and `continue`
  - explicit switch fallthrough edge shape
  - do-while back-edge plus `continue`/`break` interaction
  - merged ternary/logical control-flow lowering
  - mixed-width pointer-difference scaling in one function (`int*`, `long long*`)
- Current `13__*` targeted sweep: `23/23` passing.

## Wave 5 Additions (Negative Coverage + Extra CFG)
- Added active `13` tests:
  - `13__ir_goto_backedge`
  - `13__ir_loop_if_merge`
  - `13__ir_reject_pointer_plus_pointer`
  - `13__ir_reject_subscript_scalar_base`
  - `13__ir_reject_shift_pointer_operand`
  - `13__ir_reject_deref_non_pointer`
- Coverage intent:
  - strengthen goto/backedge and loop-merge lowering shape checks
  - add explicit compile-fail semantic guardrails so invalid operand patterns
    cannot drift into codegen (`pointer+pointer`, scalar subscript base,
    pointer shift operand, dereference of non-pointer)
- Current `13__*` targeted sweep: `29/29` passing.

## Wave 6 Additions (Calls + Signedness + More Negatives)
- Added active `13` tests:
  - `13__ir_switch_signed_cases`
  - `13__ir_fnptr_array_dispatch`
  - `13__ir_recursion_factorial`
  - `13__ir_for_continue_break_accumulate`
  - `13__ir_signed_unsigned_compare`
  - `13__ir_reject_mod_pointer_operand`
  - `13__ir_reject_bitwise_float_operand`
  - `13__ir_reject_mul_pointer_operand`
  - `13__ir_reject_shift_float_rhs`
- Coverage intent:
  - expand call-lowering and recursion IR paths
  - cover signed case values in switch lowering
  - anchor signed/unsigned comparison lowering path
  - harden invalid-operand semantic rejection for `%`, `&`, `*`, `<<`
- Current `13__*` targeted sweep: `38/38` passing.

## Wave 7 Additions (Variadic + ABI + Arity Guardrails)
- Added active `13` tests:
  - `13__ir_variadic_call_site`
  - `13__ir_nested_switch_loop_fallthrough_ctrl`
  - `13__ir_struct_by_value_transform`
  - `13__ir_mutual_recursion`
  - `13__ir_reject_nonvariadic_too_many_args`
  - `13__ir_reject_nonvariadic_too_few_args`
  - `13__ir_reject_cast_struct_return_to_int`
- Coverage intent:
  - ensure variadic call-site lowering remains stable in IR
  - stress nested switch+loop CFG with fallthrough/continue/break in one path
  - anchor struct-by-value argument/return lowering signature shape
  - harden semantic arity rejection for non-variadic calls
  - keep non-scalar cast rejection fail-closed before codegen
- Current `13__*` targeted sweep: `45/45` passing.

## Wave 8 Additions (Union/Volatile + Lvalue Negatives)
- Added active `13` tests:
  - `13__ir_union_by_value_roundtrip`
  - `13__ir_nested_switch_dispatch`
  - `13__ir_comma_chain_assign`
  - `13__ir_volatile_global_sequence`
  - `13__ir_reject_assign_non_lvalue`
  - `13__ir_reject_preinc_non_lvalue`
- Coverage intent:
  - strengthen aggregate ABI coverage to include union-by-value flow
  - deepen nested switch CFG lowering coverage
  - add comma-expression chain lowering anchor
  - deepen volatile store/load sequencing IR coverage
  - keep lvalue-constraint failures fail-closed before codegen
- Current `13__*` targeted sweep: `51/51` passing.

## Wave 9 Additions (Function-Pointer Arity Guardrails)
- Fixed semantic arity checks for function-pointer calls so fixed-signature
  targets now reject too many/too few call arguments.
- Resolved probes:
  - `13__probe_fnptr_too_many_args_reject`
  - `13__probe_fnptr_too_few_args_reject`
  - Current probe status for this slice: `blocked=0`, `resolved=2`.
- Promoted active negatives:
  - `13__ir_reject_fnptr_too_many_args`
  - `13__ir_reject_fnptr_too_few_args`
- Current `13__*` targeted sweep: `53/53` passing.

## Wave 10 Additions (Edge-Depth Roundout)
- Added active `13` tests:
  - `13__ir_switch_enum_signed_dispatch`
  - `13__ir_pointer_null_ternary_index`
  - `13__ir_while_goto_backedge_accumulate`
  - `13__ir_struct_array_field_load`
  - `13__ir_logical_ternary_merge_chain`
  - `13__ir_unsigned_shift_extract`
  - `13__ir_reject_postinc_non_lvalue`
  - `13__ir_reject_unary_plus_pointer`
  - `13__ir_reject_bitnot_float`
- Coverage intent:
  - deepen enum/signed switch dispatch lowering
  - add pointer-null compare with index path
  - stress while-loop with goto backedge control flow
  - cover struct array-field load path
  - strengthen ternary/logical merged-CFG shape
  - expand unary/lvalue semantic fail-closed negatives
- Current `13__*` targeted sweep: `62/62` passing.

## Wave 11 Additions (Function-Pointer Compatibility + Strict `%` Semantics)
- Fixed semantic typing for function designators so identifiers for functions
  carry full function-type metadata (not just return type) before decay.
- Fixed `%` operand constraints to require integer operands.
- Tightened conditional pointer compatibility for function pointers so
  incompatible signatures now fail in ternary expressions.
- Resolved probe blockers:
  - `13__probe_fnptr_ternary_decay_runtime`
  - `13__probe_mod_float_reject`
  - `13__probe_fnptr_assign_incompatible_reject`
  - Current probe status for `PROBE_FILTER=13__probe_` is now
    `blocked=0`, `resolved=10`, `skipped=0`.
- Promoted active additions:
  - `13__ir_fnptr_ternary_decay_dispatch`
  - `13__ir_reject_fnptr_assign_incompatible`
  - `13__ir_reject_mod_float_operand`
  - `13__ir_reject_fnptr_ternary_incompatible`
- Current `13__*` targeted sweep: `66/66` passing.

## Wave 12 Additions (Pointer-Domain Assignment Matrix)
- Added new pointer-domain diagnostic probes:
  - `13__probe_voidptr_to_fnptr_assign_reject`
  - `13__probe_fnptr_to_voidptr_assign_reject`
  - `13__probe_fnptr_nested_qualifier_loss_reject`
- Probe results:
  - resolved:
    - `13__probe_voidptr_to_fnptr_assign_reject`
    - `13__probe_fnptr_to_voidptr_assign_reject`
    - `13__probe_fnptr_nested_qualifier_loss_reject`
- Promoted active negatives:
  - `13__ir_reject_voidptr_to_fnptr_assign`
  - `13__ir_reject_fnptr_to_voidptr_assign`
  - `13__ir_reject_fnptr_nested_qualifier_loss`
- Current active `13` case count: `69`.

## Wave 13 Additions (Qualifier-Loss Matrix Expansion)
- Added additional function-pointer qualifier-loss probes:
  - `13__probe_fnptr_nested_volatile_qualifier_loss_reject`
  - `13__probe_fnptr_deep_const_qualifier_loss_reject`
- Probe results:
  - resolved:
    - `13__probe_fnptr_nested_volatile_qualifier_loss_reject`
    - `13__probe_fnptr_deep_const_qualifier_loss_reject`
  - current overall status for `PROBE_FILTER=13__probe_`:
    `blocked=0`, `resolved=15`, `skipped=0`.
- Promoted active negatives:
  - `13__ir_reject_fnptr_nested_volatile_qualifier_loss`
  - `13__ir_reject_fnptr_deep_const_qualifier_loss`
- Current active `13` case count: `71`.

## Wave 14 Additions (UB Signed-Overflow IR Stress)
- Added UB-tagged IR-only stress tests:
  - `13__ir_ub_signed_add_overflow_path`
  - `13__ir_ub_signed_mul_overflow_path`
  - `13__ir_ub_signed_sub_overflow_path`
- Metadata marks these as `ub: true`; they are compile/IR checks only and do
  not use runtime differential oracles.
- Coverage intent:
  - keep signed-overflow lowering paths observable in IR without asserting
    runtime behavior for undefined semantics.
  - anchor control/data-flow emission for overflow-prone add/mul/sub patterns.
- Current active `13` case count: `74`.

## Wave 15 Additions (UB Overflow-Op Matrix Expansion)
- Added additional UB-tagged IR-only stress tests:
  - `13__ir_ub_signed_shl_overflow_path`
  - `13__ir_ub_negate_intmin_path`
  - `13__ir_ub_div_intmin_neg1_path`
  - `13__ir_ub_mixed_overflow_compare_chain`
- Metadata marks all as `ub: true`; they remain compile/IR checks only.
- Coverage intent:
  - capture signed left-shift overflow lowering path.
  - capture unary negate `INT_MIN` overflow path.
  - capture divide `INT_MIN / -1` overflow path.
  - capture mixed overflow+comparison branch-chain lowering behavior.
- Current active `13` case count: `78`.

## Wave 16 Additions (UB Final Matrix Run)
- Added final UB-tagged IR-only stress tests:
  - `13__ir_ub_preinc_intmax_path`
  - `13__ir_ub_predec_intmin_path`
  - `13__ir_ub_add_assign_intmax_path`
  - `13__ir_ub_sub_assign_intmin_path`
  - `13__ir_ub_shift_count_eq_width_path`
  - `13__ir_ub_shift_count_negative_path`
- Metadata marks all as `ub: true`; they remain compile/IR checks only.
- Coverage intent:
  - anchor overflow behavior on pre-increment/pre-decrement paths.
  - anchor signed-overflow behavior on compound assign ops (`+=`, `-=`).
  - anchor shift-count UB paths (`count == width`, negative count) as explicit
    IR-shape observability cases.
- Current active `13` case count: `84`.

## Wave 17 Additions (Non-UB Shift-Width Negatives)
- Added strict fail-closed negative diagnostics tests:
  - `13__ir_reject_shift_count_negative_const`
  - `13__ir_reject_shift_count_eq_width_const`
  - `13__ir_reject_shift_count_gt_width_const`
- Coverage intent:
  - move post-UB hardening into strict diagnostics lanes for constant shift
    width validation.
  - ensure `< 0`, `== bit-width`, and `> bit-width` shift counts fail before
    IR/codegen paths.
- Current active `13` case count: `87`.

## Wave 18 Additions (Assignment-Form Integer-Op Negatives)
- Added strict fail-closed assignment negatives:
  - `13__ir_reject_shl_assign_count_negative_const`
  - `13__ir_reject_shl_assign_count_eq_width_const`
  - `13__ir_reject_shl_assign_count_gt_width_const`
  - `13__ir_reject_shr_assign_count_negative_const`
  - `13__ir_reject_shr_assign_count_eq_width_const`
  - `13__ir_reject_shr_assign_count_gt_width_const`
  - `13__ir_reject_bitand_assign_float_rhs`
  - `13__ir_reject_mod_assign_float_rhs`
  - `13__ir_reject_shl_assign_float_rhs`
  - `13__ir_reject_shl_assign_pointer_lhs`
- Coverage intent:
  - close assignment-form gaps so width checks on `<<=`/`>>=` are enforced the
    same way as plain shift expressions.
  - anchor integer-operator assignment constraints against float/pointer
    operands with explicit diagnostics.
- Current active `13` case count: `97`.

## Wave 19 Additions (`|=` / `^=` Assignment Negative Matrix)
- Added strict fail-closed negatives:
  - `13__ir_reject_bitor_assign_float_rhs`
  - `13__ir_reject_bitxor_assign_float_rhs`
  - `13__ir_reject_bitor_assign_pointer_lhs`
  - `13__ir_reject_bitxor_assign_pointer_lhs`
  - `13__ir_reject_bitor_assign_pointer_rhs`
  - `13__ir_reject_bitxor_assign_pointer_rhs`
  - `13__ir_reject_bitor_assign_struct_lhs`
  - `13__ir_reject_bitxor_assign_struct_lhs`
  - `13__ir_reject_bitor_assign_float_lhs`
  - `13__ir_reject_bitxor_assign_float_lhs`
- Coverage intent:
  - complete assignment-form bitwise negative symmetry for `|=` and `^=`.
  - enforce integer-only operand diagnostics and assignment compatibility
    diagnostics across float/pointer/struct misuse variants.
- Current active `13` case count: `107`.

## Wave 20 Additions (Arithmetic Assignment Negative Matrix)
- Added strict fail-closed negatives:
  - `13__ir_reject_add_assign_pointer_float_rhs`
  - `13__ir_reject_add_assign_pointer_rhs`
  - `13__ir_reject_add_assign_struct_lhs`
  - `13__ir_reject_add_assign_struct_rhs`
  - `13__ir_reject_sub_assign_pointer_float_rhs`
  - `13__ir_reject_sub_assign_pointer_rhs`
  - `13__ir_reject_mul_assign_pointer_lhs`
  - `13__ir_reject_div_assign_pointer_rhs`
  - `13__ir_reject_mod_assign_pointer_lhs`
  - `13__ir_reject_mod_assign_float_lhs`
- Coverage intent:
  - complete assignment-form arithmetic negative symmetry for
    `+=`, `-=`, `*=`, `/=`, `%=` misuse paths.
  - enforce invalid pointer/float/struct combinations fail closed before
    IR/codegen with stable diagnostics.
- Current active `13` case count: `117`.

## Wave 21 Additions (Assignment Diagnostics Normalization)
- Semantic behavior tightened for compound assignments:
  when compound-operator validation already emits a primary operand/shift
  diagnostic, suppress follow-on `"Incompatible assignment operands"` for that
  same node.
- Coverage impact:
  - refreshed expectations across assignment-negative manifests:
    `wave17`, `wave18`, and `wave19`.
  - keeps fail-closed behavior while reducing duplicate secondary diagnostics.
- Diagnostics policy for these lanes:
  - primary operator/shift diagnostic is required.
  - assignment-compatibility follow-on is emitted only when no primary
    compound-operator diagnostic was produced.

## Wave 22 Additions (Cast/Ternary Diagnostics Normalization Locks)
- Added explicit fail-closed diagnostics tests for normalization policy:
  - `13__ir_reject_cast_invalid_pointer_add_source`
  - `13__ir_reject_ternary_invalid_branch_suppresses_incompatible`
  - `13__ir_reject_ternary_invalid_condition_suppresses_incompatible`
  - `13__ir_reject_ternary_invalid_logical_branch_suppresses_incompatible`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitwise_float`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_pointer_plus_pointer`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_bitwise_float`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shift_width`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_call_too_few_args`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_too_few_args`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_invalid_cast`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_incompatible_assignment`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_unary_plus_pointer`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_bitnot_float`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_assign_non_lvalue`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shift_pointer`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_logical_and_struct`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_logical_or_struct`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_subscript_scalar_base`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_subscript_scalar_base`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_assign_incompatible`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_assign_incompatible`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_mod_float`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_mod_float`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_relational_struct`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_relational_struct`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_deref_non_pointer`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_deref_non_pointer`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_equality_struct`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_equality_struct`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_qualifier_loss`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_qualifier_loss`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_sub_assign_pointer_rhs`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_sub_assign_pointer_rhs`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_count_negative_const`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_count_negative_const`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_count_eq_width_const`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_count_eq_width_const`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_float_rhs`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_float_rhs`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shl_assign_pointer_lhs`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shl_assign_pointer_lhs`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_too_many_args`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_too_many_args`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_float_rhs`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_float_rhs`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_shr_assign_pointer_lhs`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_shr_assign_pointer_lhs`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_volatile_qualifier_loss`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_volatile_qualifier_loss`
  - `13__ir_reject_parser_recovery_ternary_missing_colon_then_fnptr_deep_const_qualifier_loss`
  - `13__ir_reject_parser_recovery_nested_ternary_missing_colon_then_fnptr_deep_const_qualifier_loss`
- Added manifest:
  - `tests/final/meta/13-codegen-ir-wave22-diag-normalization.json`
- Coverage intent:
  - lock cast behavior so invalid source expressions preserve the primary
    operator diagnostic without emitting follow-on cast noise.
  - lock ternary behavior so invalid condition or invalid branch diagnostics
    remain primary and no extra ternary-incompatible follow-on is emitted.
- Additional mixed-lane lock:
  - malformed ternary parse recovery is required to still surface a deterministic
    follow-on semantic operator diagnostic (`&` float operand rejection and
    pointer+pointer misuse), with parser diagnostics captured in `.pdiag`.
  - nested malformed ternary recovery is now explicitly locked to avoid
    diagnostic-count jitter in deeper recovery paths, including constant
    shift-width diagnostics.
  - malformed ternary recovery now also locks function-call arity diagnostics
    and function-pointer call-arity diagnostics under recovery.
  - malformed ternary recovery now locks cast rejection
    (`Invalid cast between non-scalar types`) as another semantic category.
  - malformed ternary recovery now locks assignment-family and unary/shift
    operator-family diagnostics under parser-recovery conditions.
  - malformed ternary recovery now locks logical-operator-family and subscript
    base diagnostics, including nested malformed recovery depth.
  - malformed ternary recovery now locks function-pointer assignment
    incompatibility and modulo-float arithmetic rejection in both shallow and
    nested malformed recovery paths.
  - malformed ternary recovery now locks relational-comparator rejection on
    struct operands and dereference non-pointer rejection across shallow and
    nested malformed recovery paths.
  - malformed ternary recovery now locks equality-comparator rejection and
    function-pointer qualifier-loss assignment diagnostics in both shallow and
    nested malformed recovery paths.
  - malformed ternary recovery now locks pointer-domain subtraction rejection
  and compound shift-width constant validation diagnostics (`<<=`, `>>=`)
  across shallow and nested malformed recovery paths.
  - malformed ternary recovery now locks assignment-form integer-operand
    rejection (`<<=` float rhs, `<<=` pointer lhs) and function-pointer
    call-arity symmetry (too few/too many) across shallow and nested lanes.
  - malformed ternary recovery now locks full shift-assignment operand symmetry
    for both `<<=` and `>>=` (float rhs + pointer lhs), plus deeper function-
    pointer qualifier-loss variants (volatile/deep-const) in shallow and nested
    lanes.
  - malformed ternary recovery now locks full compound shift-width constant
    validation symmetry (`<<=` negative/eq/gt and `>>=` negative/eq/gt) and
    bitwise-assignment integer-domain rejection for `|=`/`^=` (float rhs +
    pointer lhs) in both shallow and nested lanes.
  - malformed ternary recovery now also locks remaining bitwise-assignment
    symmetry (`|=`/`^=` float lhs + pointer rhs) and extends assignment-family
    fail-closed coverage with `&=` float rhs and `%=` (float rhs + pointer lhs)
    across shallow and nested lanes.
  - malformed ternary recovery now also locks `%=` float-lhs and `|=`/`^=`
    struct-lhs misuse in shallow and nested lanes, completing parity with these
    non-recovery negatives.
  - malformed ternary recovery now also locks arithmetic-assignment asymmetry
    lanes (`*=` pointer lhs, `/=` pointer rhs, `+=` struct lhs, `-=` pointer
    lhs + float rhs) and expanded `&=` parity (`pointer lhs/rhs`, `struct lhs`)
    in shallow and nested lanes.
  - malformed ternary recovery now also locks remaining `+=`/`-=` symmetry
    variants (`+=` pointer rhs, `+=` pointer lhs+float rhs, `+=` struct rhs,
    `-=` struct lhs, `-=` struct rhs) in shallow and nested lanes.
  - malformed ternary recovery now also locks remaining small `&=` parity edges
    (`float lhs`, `struct rhs`) in shallow and nested lanes.
- Current active `13` case count: `237`.

## 13 Status
- Stability: `stable` (current baseline).
- Active blockers: none.
- Full suite status: `python3 tests/final/run_final.py` green after wave 41.

## Open 13 Gaps (Deferred/Optional)
- Optional doc hygiene:
  collapse repeated recovery bullets into a short per-family checklist table to
  keep this doc maintainable as the suite continues to grow.
- Optional future depth:
  add any extra rhs/lhs symmetry variants that are intentionally outside current
  non-recovery coverage policy (not required for current stable baseline).
