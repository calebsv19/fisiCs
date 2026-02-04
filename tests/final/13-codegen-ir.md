# Codegen / IR Correctness

## Scope
Lowering semantics once IR/codegen is enabled.

## Validate
- Correct addressing for locals/globals.
- Struct layout/offsets and short-circuit lowering.
- Ternary and switch lowering.

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
5) `13__ir_ptr_arith`
   - pointer arithmetic lowering.
6) `13__ir_struct_access`
   - struct field access lowering.
7) `13__ir_logical_not`
   - logical not lowering.
8) `13__ir_bitwise`
   - bitwise ops lowering.
9) `13__ir_short_circuit_side_effect`
   - short-circuit with side effects.
10) `13__ir_compare_branch`
   - comparison lowering to boolean.
11) `13__ir_global_init`
   - global initializer emission.

## Expected Outputs
- IR expectations (`.ir`) for codegen output.
