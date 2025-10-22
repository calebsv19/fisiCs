# Code Generation

Experimental LLVM IR emitter that lowers the semantic AST into executable IR.

## Files

- `code_gen.h`
  - Public surface now exposes a `CodegenContext` wrapper instead of global LLVM objects.
  - `codegen_context_create()` creates the context (LLVM context, module, builder, loop stack, scope chain) and now accepts the optional `SemanticModel*` so semantic analysis results can be reused when lowering IR.
  - `codegen_generate()` walks the AST and emits IR into the supplied context.
  - `codegen_get_module()` lets callers inspect/dump the produced module; destroy everything with `codegen_context_destroy()`.
- `code_gen.c`
  - Implements the context helpers (scope management, loop-target stack) plus all `codegen*` routines.
  - Each node-specific emitter still relies on LLVM C bindings (arithmetic via `LLVMBuildAdd/Sub`, control flow via `LLVMBuildCondBr`, switches via `LLVMBuildSwitch`, etc.), but now threads the context explicitly instead of touching globals.
  - A lightweight pre-pass registers global variables, function prototypes, and tagged struct/union types before the body emitters run so the main walk can assume symbols are already known.
  - String/number/char literals become `LLVMConstInt`/`LLVMBuildGlobalStringPtr`; identifiers currently resolve through the scope table first, falling back to globals (`LLVMGetNamedGlobal`) until richer symbol data is wired in.
  - Ternary expressions are implemented with explicit merge blocks and PHI nodes, illustrating the pattern other conditional emitters follow.
- `codegen_types.c` / `codegen_types.h`
  - Maps the parser's `ParsedType` (primitives, pointers, basic structs/unions/enums) to LLVM types using the shared `CodegenContext` cache helpers declared in `codegen_internal.h`.
  - Currently handles primitive widths, pointer depth, and function pointers; opaque struct/union names are cached for later population.
- `codegen_internal.h`
  - Private helper surface shared across codegen implementation files (accessors for the LLVM context/module/builder and a small named-type cache).

## Usage

```
SemanticModel* sem = analyzeSemanticsBuildModel(astRoot, ctx, /*takeContextOwnership*/ false);
CodegenContext* cg = codegen_context_create("my_module", sem);
LLVMValueRef rootVal = codegen_generate(cg, astRoot);
LLVMDumpModule(codegen_get_module(cg));
codegen_context_destroy(cg);
semanticModelDestroy(sem);
```

`main.c` controls whether this phase runs via `ENABLE_CODEGEN`. The current implementation still assumes integer-centric types and basic storage; extend the individual `codegen*` functions as the AST evolves.
