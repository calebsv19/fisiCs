# Failing or Suspicious Behavior (make run, include/test.txt)

====== Lexer, literals, and tokens (in progress) ======
- [FAIL] Integer literal values/codegen: octal/hex/suffixes parse, but IR stores wrong values (e.g., `0x1F` -> `1`, `0x7fffffffffffffff` -> `7`) and `18446744073709551615ULL` becomes `poison`.
- [FAIL] Integer overflow diagnostics: `18446744073709551616ULL` produces no diagnostic and emits `poison`.
- [FAIL] Multi-character character constants: `'ab'` tokenizes as `'a'` + identifier `b` (parse error).
- [PASS] Escape sequences: `\x41`, `\101`, `\'`, `\\` parse and codegen to expected values.
- [PARTIAL] Wide/UTF prefixes: `L'A'` and `L"hi"` parse; `u8"hi"` accepted but treated as narrow `char*`.
- [PASS] Comment stripping works for token adjacency (`5/*comment*/+2`).
- [FAIL] Backslash-newline line splicing not honored for identifiers/numbers (`1\
23`, `a\
b`).
- [PASS] Alternative tokens `%:` and `%:%:` work for `#`/`##` in macros.

====== Preprocessor (in progress) ======
- [PASS] Macro self-expansion suppression + rescan: `#define A A` yields `#if A` false; `#define X Y` + `#define Y 123` expands to 123.
- [FAIL] `defined` in macro-expanded contexts: `#define CHECK(x) defined(x)` then `#if CHECK(FOO)` evaluates false (hits `#error`).
- [PASS] Empty `__VA_ARGS__` works in `#define F(...) __VA_ARGS__` with `F()` used in expression.
- [PASS] `#pragma once` respected (including `pragma_once.h` twice does not redeclare typedef).
- [PARTIAL] `__FILE__`, `__DATE__`, `__TIME__` expand to strings, but `#line 100` + `#if __LINE__ != 100` fails (line mismatch); `__LINE__`/`#line` tracking appears off.
- [PASS] Macro-generated `#include` operands work for both `<pp_sys.h>` and "shadow.h" forms.

====== Declarations, types, and qualifiers (in progress) ======
- [FAIL] Abstract declarators: `sizeof(int (*)[3])` parses as `sizeof(int)` and `int (*p)[3]` is treated as `int* [3]` (array of pointers). Pointer-to-array abstract declarators are misparsed.
- [FAIL] Function pointer typedef chains: typedefs parse, but file-scope init `G g = foo;` is rejected as non-constant (should be allowed).
- [PARTIAL] VLA/`static`/`const`/`restrict` array parameter qualifiers are parsed but ignored/flattened (AST shows fixed arrays; qualifiers lost). No static-size enforcement.
- [PASS] `restrict` pointer qualifier parses and type-checks (e.g., `int * restrict p`).
- [PASS] Bitfield width constraint checked: `int b:33;` errors (location reported at 0:0).

====== Expressions and operators (in progress) ======
- [PASS] Cast vs parenthesized expression resolves with typedef types (`(T)(x)` cast vs `(x)` grouping).
- [FAIL] Comma inside ternary true-expression (`0 ? 1,2 : 3`) fails to parse (expects ':' before comma).
- [FAIL] `sizeof(int[2])` parsed as `sizeof(int)` (array dimension dropped); `sizeof((const int[]){1,2})` parsed but comparisons use wrong size.
- [PASS] Nested short-circuit side effects parse and codegen (&&/|| with assignments) without errors.

====== Lvalues, rvalues, and conversions (in progress) ======
- [PASS] Assignment to `const` struct field is rejected (`Left operand of '=' must be a modifiable lvalue`).
- [PARTIAL] Address-of restrictions: `&register` is rejected, but `&bitfield` is currently allowed (no diagnostic).
- [FAIL] Pointer vs integer comparison (`p == 1`) emits no diagnostic (expected warning/error for non-null constant).
- [PARTIAL] Usual arithmetic conversions for mixed signed/unsigned widths not strongly validated; `sizeof(u+s)` comparison passes but doesn't prove signedness behavior.

====== Initializers and object layout (in progress) ======
- [FAIL] Designator reset across aggregates: `int a[3] = { [1]=2, 3, [0]=1 };` emits `[1,2,0]` (missing expected `a[2]=3`).
- [FAIL] Brace elision for multi-dimensional aggregates: `int m[2][3] = {1,2,3,4,5,6};` errors as too many initializers (treats as 1D).
- [PASS] String initializer length diagnostics: `char s[3] = "abcd";` errors with correct size info.
- [PASS] Static storage init requires constant expr: `int g = foo();` rejected.
- [PARTIAL] Struct/union layout/offset checks not directly validated yet (no explicit size/offset assertions).

====== Statements and control flow (in progress) ======
- [PASS] Duplicate `case` labels detected with error + warning.
- [PASS] `goto` into scope with VLA/initialized variable rejected (error, location 0:0).
- [PARTIAL] Label redeclaration: warns "label redefined" but also throws parse error; no clear semantic error.
- [FAIL] `break` outside loop/switch not diagnosed (only unreachable + control-reaches-end warnings).

====== Scoping, linkage, and redeclarations (in progress) ======
- [PASS] `extern` vs `static` linkage mismatch diagnosed (`Conflicting linkage for variable`).
- [PASS] Tag namespace vs typedef name coexistence works (`typedef Foo` + `struct Foo`).
- [PASS] K&R-style declaration (`int f();`) compatible with prototype/definition.
- [PARTIAL] Tentative definitions across multiple translation units not exercised (single-TU only).

====== Functions and calls (in progress) ======
- [FAIL] Non-prototype call promotions: `int f();` then `f(1.5)` generates IR call to `f(double)` even though `f` expects `i32` (ABI/type mismatch).
- [PASS] Variadic function calls compile; varargs used in IR for extra arguments.
- [PASS] Large struct return-by-value works (struct with 4 longs returned/assigned).
- [FAIL] Incompatible function pointer assignment (`int (*)(int)` = `int (*)(double)`) not diagnosed.

====== Codegen and lowering (in progress) ======
- [FAIL] Volatile loads/stores not marked `volatile` in IR (volatile var loads/stores emit plain `load`/`store`).
- [PASS] Switch lowering with fallthrough emits `switch` in IR; fallthrough to `default` works.
- [PASS] Ternary lowering works for struct and pointer operands (phi + stores).
- [PASS] VLA allocation emits runtime-sized `alloca` (e.g., `alloca i32, i32 %n`).

====== Runtime surface and builtins (in progress) ======
- [FAIL] `offsetof` macro not supported: `offsetof(struct S, b)` fails to parse.
- [PASS] `<stdarg.h>`/`va_list`/`va_start`/`va_arg`/`va_end` compile and lower to LLVM intrinsics.
- [PARTIAL] Additional system headers not exercised beyond stdarg/stddef (future coverage needed).

====== Diagnostics (in progress) ======
- [FAIL] Recovery in mixed declaration/statement blocks is weak: `int x = ; int y = 3;` drops `y` and cascades with undeclared identifier.
- [FAIL] Diagnostics after macro expansion point to later tokens (error at `return` line instead of macro expansion site).
- [PASS] Undeclared identifier emits hint (`Hint: foo`).
- [PARTIAL] Duplicate-diagnostic suppression not evaluated here (global env toggle exists but not exercised in this category).

====== External project validations (in progress) ======
- [PASS] `/Users/calebsv/Desktop/CodeWork/test/src/core/string_view.c` compiles clean (no Error/Warning output).
- [PARTIAL] Other external codebases not exercised in this pass.
