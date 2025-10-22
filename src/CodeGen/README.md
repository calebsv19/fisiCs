# Code Generation

Experimental LLVM IR emitter that lowers the semantic AST into executable IR.

## Files

- `code_gen.h`
  - Public surface: `initializeLLVM()` bootstraps the global module and builder.
  - `codegen(ASTNode*)` dispatches on AST node types and forwards to the per-node emitters listed below.
  - Declares helpers for common constructs (`codegenProgram`, `codegenIfStatement`, `codegenFunctionDefinition`, literals, assignments, loop nodes, etc.), plus struct utilities such as `getFieldIndexFromStruct` and `codegenCase`.
- `code_gen.c`
  - Holds the concrete LLVM emission logic. Maintains `TheModule`, a shared `LLVMBuilderRef`, and scoped loop targets for `break`/`continue`.
  - Each `codegenX` builds the appropriate IR using the C bindings: arithmetic via `LLVMBuildAdd/Sub/Mul`, control flow with appended basic blocks and `LLVMBuildCondBr`, switch lowering with `LLVMBuildSwitch`, and so on.
  - String/number/char literals become `LLVMConstInt`/`LLVMBuildGlobalStringPtr`; identifiers currently resolve through `LLVMGetNamedGlobal`.
  - Ternary expressions are implemented with explicit merge blocks and PHI nodes, illustrating the pattern other conditional emitters follow.

## Usage

```
initializeLLVM();
LLVMValueRef rootVal = codegen(astRoot);
LLVMDumpModule(TheModule);
```

`main.c` controls whether this phase runs via `ENABLE_CODEGEN`. The current implementation assumes integer-centric types and global variable storage; extend the individual `codegenX` functions as the AST evolves.
