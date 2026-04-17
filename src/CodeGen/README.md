# Code Generation

Experimental LLVM IR emitter that lowers the semantic AST into executable IR. Types are reconstructed directly from the `ParsedType` derivation stack, so nested declarators (arrays of function pointers, pointer-to-array returns, VLAs inside structs, etc.) no longer rely on bespoke AST nodes.

## Files

- `code_gen.h`
  - Public surface for consumers: create/destroy the `CodegenContext`, drive `codegen_generate`, and fetch the final `LLVMModuleRef`.
- `code_gen.c`
  - Thin entry point that seeds a fresh scope, kicks off global predeclarations, and dispatches to the shared helpers.
- `codegen_private.h`
  - Internal umbrella header that exposes the `CodegenContext` layout, scope/loop/label helpers, and every emitter prototype so the split source files can talk to one another.
- `codegen_context.c`
  - Owns lifecycle concerns: LLVM context/module/builder creation, semantic-model wiring, scope stack, loop stack, named-type cache, label book-keeping, and public context accessors.
- `codegen_helpers.c`
  - Reusable expression utilities (value classification, truthiness coercion, integer predicate selection, `cg_cast_value`, ternary arm merge/PHI builder, and expression-type resolution/call-result refinement).
- `codegen_layout_abi.c`
  - Owns ABI/lowering and memory-layout helpers: parameter lowering, aggregate ABI pack/unpack for returns, size/align bridging, pointer element-type sizing, VLA byte sizing, entry allocas, load/store helpers, and pointer offset/difference builders.
- `codegen_declarations.c`
  - Handles the predeclare coordinator path: collecting parameter LLVM types, ensuring functions/globals exist in the module, registering struct symbols, and walking the AST/SemanticModel to predeclare everything prior to body emission.
- `codegen_const_initializers.c`
  - Owns compile-time LLVM constant materialization for global predeclaration, including folded scalar constants, constant string globals, and constant array/struct initializer builders used by global symbol setup.
- `codegen_aggregate_access.c`
  - Owns aggregate-addressing helpers: array element pointer construction, struct field pointer construction, flexible-array/member layout lookups, and aggregate-to-LLVM struct metadata resolution reused by the struct/lvalue lowering path.
- `codegen_local_decls.c`
  - Owns local variable declaration lowering: stack/static storage setup, VLA extent evaluation, inline struct-definition pre-emission, and local initializer materialization for scalars, arrays, and compound literals.
- `codegen_initializers.c`
  - Emits designated/compound initializer stores for arrays/structs/scalars, reusing the expression helpers to fill aggregates element-by-element. Zero-inits use `memset`; aggregate copies use `memcpy` with semantic size/align hints.
- `codegen_structs.c`
  - Tracks struct metadata (`StructInfo` cache), bridges to the semantic/type-cache layer, and implements struct/union definition lowering plus the lvalue coordinator that delegates aggregate-address calculations to `codegen_aggregate_access.c`.
- `codegen_expr.c`
  - Shared expression helper coordinator for the `CodeGen/Expr/` bucket: complex/atomic/value-shape helpers, common integer/float promotion utilities, and the shared private bridge consumed by the split expression emitters.
- `Expr/codegen_expr_access.c`
  - Owns identifier/member/array/pointer access, compound literal materialization, and aggregate-address reuse for lvalue-heavy expression lowering.
- `Expr/codegen_expr_literals.c`
  - Owns scalar/string literal lowering plus `sizeof`/`alignof`/heap-allocation expression nodes.
- `Expr/codegen_expr_unary_assign.c`
  - Owns unary-expression lowering and assignment/update semantics, including bitfield-aware lvalue stores.
- `Expr/codegen_expr_scalar_control.c`
  - Owns cast/comma/ternary/logical short-circuit lowering and the PHI merge path for scalar control expressions.
- `Expr/codegen_expr_binary.c`
  - Owns binary arithmetic/comparison/pointer-difference lowering, including complex arithmetic and integer/float promotion handling.
- `Expr/codegen_expr_call_support.c`
  - Owns call-signature support helpers: default promotions, call-argument preparation, varargs intrinsics, and builtin call rewrite support.
- `Expr/codegen_expr_call_builtins.c`
  - Owns builtin/special-case call lowering for atomics, `va_*`, fortified libc rewrites, allocation/object-size intrinsics, floating-point constants, and `__builtin_expect`.
- `Expr/codegen_expr_calls.c`
  - Owns the ordinary function-call emission path: semantic signature recovery, opaque-pointer fallback inference, ABI-adjusted argument preparation, and aggregate return unpacks.
- `codegen_control_flow.c`
  - Owns CFG-oriented statement lowering: `if`/loop/switch lowering, break/continue dispatch, label/goto handling, and the branch/merge helpers reused by the statement coordinator.
- `codegen_stmt.c`
  - Statement/function coordinator: programs, blocks, statement expressions, typedef/enum registration, returns, and top-level function bodies. Local storage emission now routes through `codegen_local_decls.c`, and CFG-heavy branch lowering routes through `codegen_control_flow.c`.
- `codegen_dispatch.c`
  - Central `codegenNode` dispatcher that routes AST node kinds to the appropriate statement/expression emitter.
- `codegen_types.c` / `codegen_types.h`
  - Maps `ParsedType` derivations to LLVM types by replaying pointer/array/function entries in order before applying any residual legacy pointer depth.
- `codegen_type_cache.c` / `codegen_type_cache.h`
  - Caches semantic-model derived struct layouts so the LLVM lowering phase can ask for struct field shapes without reparsing the AST.
- `codegen_model_adapter.[ch]`
  - Bridges the semantic model to LLVM type info; used when the code generator needs additional details about analyzed symbols.
- `codegen_internal.h`
  - Legacy accessors kept for compatibility with other submodules (wrapper around `codegen_private.h` providing context getters and named-type cache hooks).

## Usage

```
SemanticModel* sem = analyzeSemanticsBuildModel(astRoot, ctx, /*takeContextOwnership*/ false);
CodegenContext* cg = codegen_context_create("my_module", sem);
LLVMValueRef rootVal = codegen_generate(cg, astRoot);
LLVMDumpModule(codegen_get_module(cg));
codegen_context_destroy(cg);
semanticModelDestroy(sem);

# Verification

Set `CODEGEN_VERIFY=1` in the environment when invoking `./compiler` (or `make run/tests`) to ask LLVM to run `LLVMVerifyFunction` after each function body is emitted. Any verification failures are printed to stderr so malformed IR is caught early while iterating on new control-flow paths.
```

`main.c` controls whether this phase runs via `ENABLE_CODEGEN`. Setting `DISABLE_CODEGEN=1` in the environment temporarily skips LLVM emission—handy when iterating on parser/semantics without worrying about backend regressions. Flags `--target <triple>` and `--data-layout <layout>` (or env vars) seed the LLVM module; intptr width and pointer sizes reflect the active data layout.

## Declarator status & recent backend work

- `cg_expand_parameters` flattens multi-name declarators into real LLVM arguments, honours `(void)` signatures, and drives both function definitions and prototypes.
- Struct/union lowering consults the semantic fingerprints for layout, so anonymous tags and typedef-only names resolve without touching legacy AST caches. Size/align bridge pulls from `layout.c` (packed/aligned, ABI profiles).
- Globals, locals, and compound literals store/decay using the semantic element types, so pointer arithmetic and initializer checks see identical LLVM shapes.
- ABI return coercion, parameter lowering, load/store volatility checks, and pointer-difference math now route through `codegen_layout_abi.c`, keeping `codegen_helpers.c` focused on expression typing and conversion semantics.
- Array/pointer lvalues decay via `buildArrayElementPointer`, which detects raw `[N x T]` values, stores them into temporaries when needed, and GEPs using parsed aggregate hints. Pointer arithmetic, pointer comparisons, `&`/`[]` expressions survive the LLVM stage; pointer diff uses element size to compute `ptrdiff_t`.
- Ternary lowerer merges arm types and builds a PHI; switch lowering chooses jump table vs chained branches and preserves fallthrough.
- Aggregate copies lower to memcpy; zero-init lowers to memset. Target triple/data layout flow into the LLVM module, and `intptr` sizing is derived from the active layout.
