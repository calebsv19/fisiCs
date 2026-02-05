# Functions: Calls & Prototypes

## Scope
Function types, parameter adjustments, and call checking.

## Validate
- Prototype vs non-prototype calls (if supported).
- Variadic calls and default argument promotions.
- Return type matching and array/function decay.

## Acceptance Checklist
- Function prototypes enforce parameter types.
- Array/function parameters decay correctly.
- Variadic calls accept extra arguments and promote types.
- Return types are checked for compatibility.

## Test Cases (initial list)
1) `11__prototype_call`
   - Correct and incorrect prototype usage.
2) `11__no_prototype_call`
   - Old-style declaration call lowers without ABI mismatch.
3) `11__variadic_call`
   - Variadic function with mixed argument types.
4) `11__param_decay`
   - Array and function parameter adjustment.
5) `11__return_type_mismatch`
   - Return type incompatibility diagnostic.
6) `11__void_return_value`
   - Returning a value from void function.
7) `11__function_pointer_assign`
   - Incompatible function pointer assignment diagnostic.

## Expected Outputs
- AST/Diagnostics goldens for call checking.
- IR golden for `11__no_prototype_call`.
