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
  - Reusable expression utilities (value classification, truthiness coercion, pointer arithmetic, pointer difference, integer predicate selection, `cg_cast_value`, etc.). Pointer element types are determined by peeling `ParsedType` derivations; VLAs use the derivation metadata to compute extents.
- `codegen_declarations.c`
  - Handles global prepasses: collecting parameter LLVM types, ensuring functions/globals exist in the module, registering struct symbols, and walking the AST/SemanticModel to predeclare everything prior to body emission.
- `codegen_initializers.c`
  - Emits designated/compound initializer stores for arrays/structs/scalars, reusing the expression helpers to fill aggregates element-by-element.
- `codegen_structs.c`
  - Tracks struct metadata (`StructInfo` cache), bridges to the semantic/type-cache layer, builds struct field pointers/array element pointers, and implements struct/union definition lowering plus lvalue helpers.
- `codegen_expr.c`
  - Expression-level IR generators: arithmetic/logic, casts, literals, compound literals, member/array/pointer access, function calls, and heap allocation nodes.
- `codegen_stmt.c`
  - Statement/control-flow lowering: programs, blocks, declarations, conditionals, loops, switches, returns/break/continue, typedef/enum lowering, labels/gotos, and top-level function bodies. Variable-length arrays allocate based on the derivation-provided extents.
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

`main.c` controls whether this phase runs via `ENABLE_CODEGEN`. Setting `DISABLE_CODEGEN=1` in the environment temporarily skips LLVM emission—handy when iterating on parser/semantics without worrying about backend regressions.
