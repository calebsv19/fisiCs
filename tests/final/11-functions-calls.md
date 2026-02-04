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
2) `11__variadic_call`
   - Variadic function with mixed argument types.
3) `11__param_decay`
   - Array and function parameter adjustment.
4) `11__return_type_mismatch`
   - Return type incompatibility diagnostic.
5) `11__void_return_value`
   - Returning a value from void function.

## Expected Outputs
- AST/Diagnostics goldens for call checking.
