# Passing Behavior (make run, include/test.txt)

These items compiled with no semantic or codegen errors during iterative `make run` tests.

====== Translation phases & includes ======
- Translation units, function definitions, prototypes, and calls parse/compile.
- Local `#include "shadow.h"`, system-style `#include <pp_sys.h>`, and `#include <stdio.h>` resolve and parse.
- `#line` directives remap logical file/line, including macro-expanded filenames.
- Deterministic output for identical inputs (AST/IR match across runs).
- Include search order: quoted includes prefer same-dir; `<...>` searches `INCLUDE_PATHS` before `SYSTEM_INCLUDE_PATHS`.

====== Lexer, literals, and tokens ======
- Adjacent narrow string literal concatenation (e.g., `"ab" "cd"`).
- Adjacent wide string literal concatenation (e.g., `L"hi" L"!"`).
- Wide string literals (`L"..."`) lower to `wchar_t` arrays.
- String literal escape sequences (`\n`, `\t`, `\"`) parse and codegen.
- Numeric literal suffixes for integers (`u`, `L`, `ULL`) and floats (`f`, `L`), plus hex float literals (`0x1.8p1`).
- Complex/imaginary literals and basic ops (`_Complex double`, `2.0i`, `+`, `-`, `==`).

====== Preprocessor ======
- Object-like macros, function-like macros, variadic macros, `#if/#else` expressions.
- `#` stringize and `##` token-paste.
- Nested macro expansion and `#undef`/redefine.
- Macro arguments containing commas/parentheses.
- `#line` directive accepted.
- Macro-expanded `#include` operands (e.g., `#define HDR "shadow.h"` then `#include HDR`).
- Stringized `#include` operands with tokenized headers (e.g., `#define HDR shadow.h` + `#define STR(x) STR2(x)` + `#include STR(HDR)`).
- Include guards (re-including `shadow.h` preserves `SHADOW_VAL`).
- `#error` triggers an error (surfaced in semantic diagnostics).

====== Declarations, types, and qualifiers ======
- Basic types and qualifiers: `const`, `volatile`, `register`, `static`, `inline`, `_Bool`.
- Pointer qualifiers on pointer levels preserved in output/types (`int *const`, `int *restrict`, `int *volatile`).
- Declarations mixed with statements inside blocks (C99-style).
- Pointer typedef aliases resolve correctly (e.g., `typedef int *intptr;`).
- Typedef’d array types preserve array semantics in codegen/initialization (e.g., `typedef int arr3[3]; arr3 a = {1,2,3};`).
- Array declarators: arrays of pointers (`int *arr[N]`).
- Array declarators: arrays of pointers to arrays with deref indexing (e.g., `int (*p[2])[3]; return (*p[0])[1];`).
- Pointer-to-array declarators can be indexed (e.g., `int (*arrp)[2]; return arrp[0][1];`).
- Structs/unions: definitions, aggregate init, member access `.` and `->`, return-by-value.
- Structs/unions: nested aggregates with aggregate init.
- Struct assignment between compatible struct types.
- Struct arguments passed by value compile and are accessed correctly.
- Struct return-by-value from locals, compound literals, and pointer dereferences compiles.
- Tag namespaces: `struct`/`union`/`enum` tags can reuse the same identifier without conflicts (e.g., `struct T` and `enum T`).
- Enums: tagged enum usage (`enum Mode m;`) and enum constants used as values/codegen.
- Flexible array member syntax parses and codegen is emitted.
- Bitfields (parse + codegen for read/write).
- Bitfield layout/packing: `sizeof(struct { unsigned a:3; signed b:5; })` returns 4.
- Zero-width bitfields parse and codegen.
- Bitfield read-modify-write uses masking for single-field updates.

====== Expressions and operators ======
- Scalar and compound assignments, pre/post `++/--`, unary `&`/`*`/`!`, binary arithmetic/bitwise/logical operators, shifts, ternary `?:`.
- Nested short-circuit expressions with side effects (e.g., `(x && (y = 4)) || (x || (y = 5))`).
- `++/--` on struct members and array elements works (complex lvalues).
- Comma operator in expressions (e.g., `(x = 3, x + 2)`).
- Double-pointer dereference (`int **pp; return **pp;`).
- `sizeof` on types and expressions, including runtime `sizeof` on direct VLA objects.
- `sizeof` on dereferenced pointer-to-VLA uses runtime VLA size (e.g., `int (*p)[n]; sizeof *p`).
- `sizeof` on string literals returns the literal size (e.g., `sizeof("hi")` is 3).
- `sizeof` on array compound literals uses the inferred array size (e.g., `sizeof((int[]){1,2,3})`).
- `sizeof` on struct member access via pointer (e.g., `sizeof(((struct S*)0)->b)`) uses field size.
- `sizeof` applied to function types is rejected (diagnostic emitted).
- `sizeof` on bitfield members is rejected (e.g., `sizeof(((struct S*)0)->a)` for a bitfield).

====== Lvalues, rvalues, and conversions ======
- Mixed signed/unsigned arithmetic and comparisons compile (e.g., `unsigned` vs `int`).
- Integer promotions applied for small integer arithmetic (e.g., `unsigned char + short` promotes to `int`).
- Numeric casts between floating-point and integer types (e.g., `(int)3.25`, `(int)f`).
- Pointer difference uses element-size scaling (e.g., `q - p` for `int*` divides by 4).
- Pointer comparisons (including ordering) and casts between pointers and integers.
- Void pointer arithmetic is rejected (e.g., `void *p; p = p + 1;`).
- Array-to-pointer decay, pointer arithmetic, and array indexing.

====== Initializers and object layout ======
- Static local variables emit internal globals with constant initializers (persist across calls).
- Designated initializers (struct + array) and compound literals.
- Designated array initializers inside nested aggregates parse and lower (e.g., `.a = { [1] = 4, [0] = 3 }`).
- Chained array+field designators parse and lower (e.g., `struct S a[2] = { [1].b = 5, [1].a = 3 };`).
- File-scope compound literals used in pointer initializers lower to static storage (e.g., `int *p = (int[]){1,2,3};`).
- File-scope pointer initialized from a compound literal address is accepted (e.g., `struct S *p = &(struct S){7};`).
- Multi-dimensional array initializers accept nested braces (e.g., `int a[2][3] = {{1,2,3},{4,5,6}};`).
- Union designated initializers store the selected field type (e.g., `.f = 1.5f` for `union { int i; float f; }`).
- String literal array initialization (`char s[N] = "..."`) compiles.
- Global initializers with constant expressions (e.g., `int g = 1 + 2;`) compile; non-constant ones are rejected.
- Global initializers with ternary and `sizeof` constant expressions compile.
- Ternary merges mixed arithmetic types using floating-point when needed (e.g., `int` vs `double`).
- Ternary with pointer operands of the same type (e.g., `x ? (int*)0 : &x`).
- Ternary with struct/union operands of the same type (e.g., `cond ? a : b` for `struct S`).
- VLAs (variable length arrays) with runtime sizing.

====== Statements and control flow ======
- `if/else`, `for`, `while`, `do/while`, `break`, `continue`, `goto`, labels.
- `goto` into scope with initialized variables is rejected (jumping past initializers).
- `for` loop init/increment clauses accept comma expressions (e.g., `for (i=0, j=0; ...; i++, j++)`).
- `switch` case labels accept enum constants/identifiers and constant expressions; default-only switches codegen with proper terminators.
- `switch` fallthrough into `default` is accounted for in reachability (no "control reaches end" error when default returns).

====== Scoping, linkage, and redeclarations ======
- Shadowing across nested scopes (block and global).
- Tentative definitions (multiple `int x;` in a TU) compile without redefinition errors.
- File-scope `extern` declaration followed by a definition in the same TU is allowed.
- Conflicting linkage (e.g., `extern` then `static` for same global) is rejected.
- Duplicate definitions with initializers are rejected (e.g., `int x=1; int x=2;`).
- Incompatible function redeclarations are diagnosed (e.g., `int f(int); int f(double);`).

====== Functions and calls ======
- Function-to-pointer decay works in argument passing, including typedef'd function types; `&function` yields a function pointer.
- Function pointers (single function pointer calls).
- Arrays of function pointers with initializer (e.g., `int (*funcs[2])(int) = {inc, dec};`).
- Variadic functions parse/type-check and calls with extra args compile.
- `va_list` usage compiles (`va_start`, `va_arg`, `va_end`).
- Old-style (K&R) function definitions parse and compile.
- Function prototypes with parameter names omitted (types-only prototypes) resolve correctly.
- Array parameters accept bracket qualifiers and `static` size (e.g., `int f(int a[static 3], int b[const 2])`).
- Array parameters accept `[*]` VLA size (e.g., `void g(int a[*])`).
- Functions returning pointer types compile and can be called/dereferenced.
- Functions returning pointer-to-array types compile and can be called/dereferenced.
- Function returning array types are rejected at parse time (invalid in C99).
- Inline asm statements parse and type-check (currently lowered as no-op in codegen).

====== Codegen and lowering ======
- Struct/union return-by-value and argument passing lower correctly.
- Flexible array member codegen is emitted.
- Bitfield read-modify-write uses masking for single-field updates.

====== Runtime surface and builtins ======
- `_Bool`/`bool` size is 1 (`sizeof(_Bool)`, `sizeof(bool)`).
- macOS SDK typedef chains for `va_list` (via `<machine/types.h>` + `<sys/_types/_va_list.h>`) compile without redefinition errors.
- Builtin secure-check intrinsics compile/call (`__builtin___snprintf_chk`, `__builtin___memset_chk`, `__builtin___strncpy_chk`) verified in prior `make run`.

====== Diagnostics ======
- Malformed initializers/expressions emit diagnostics without crashing the parser.
- Diagnostics de-dup across repeated frontend analyses when `FISICS_DIAG_DEDUP_GLOBAL=1` (verified by `tests/unit/frontend_api_diag_dedup.c`).

====== External project validations ======
- `/Users/calebsv/Desktop/CodeWork/test/src/system/event_bus.c` compiles with `-I/Users/calebsv/Desktop/CodeWork/test/include` and no realloc/free diagnostics.
- `/Users/calebsv/Desktop/CodeWork/test/src/core/string_view.c` and `/Users/calebsv/Desktop/CodeWork/test/src/io/ini.c` no longer emit modifiable-lvalue or `++` lvalue diagnostics.
