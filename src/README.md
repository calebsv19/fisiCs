# Source Overview

This tree implements a single-pass C front-end with optional LLVM IR emission. Control flows through:

- **Driver** (`main.c`) configures `CompilerContext` (include graph, target triple, data layout), then executes lexing → preprocessing → parsing → semantic analysis (and optional code generation/AST dumps).
- **Lexer** tokenises the raw buffer into `Token` objects (keyword lookup via an auto-generated gperf table).
- **Preprocessor** resolves includes (cached, pragma once/guards, dependency JSON), expands macros, and evaluates `#if/#elif` conditionals before handing a `TokenBuffer` to the parser; preservation mode keeps directive stubs.
- **Parser** consumes tokens into an annotated AST. Pratt and recursive-descent expression engines coexist, sharing helper utilities for type probing, declarations, and designators.
- **AST** defines the node model plus printers for debugging.
- **Syntax** runs semantic checks: storage/linkage, tentative defs, deep qualifier checks, incomplete types, bitfields, const-eval, switch/varargs checks, layout inference (packed/aligned, ABI profile), and lvalue/initializer validation.
- **Compiler** tracks typedef/tag namespaces, tag fingerprints/layout flags, include graph, and target/data-layout strings for later phases.
- **CodeGen** lowers AST nodes into LLVM IR blocks using the semantic layout/info; pointer arithmetic, switches, ternaries, memcpy/memset, and pointer diff are supported.

## Files in this directory

- `main.c` — Pipeline driver (`lex` → `preprocess` → `parse` → `semantics` → optional LLVM). Flags: `--preserve-pp`, `--emit-deps-json path`, `--target <triple>`, `--data-layout <layout>`, `--preprocess=internal|external`, `--preprocess-cmd <tool>`, `--preprocess-args "<args>"`.

## Subdirectories of note

- `CodeGen/` — LLVM IR backend (see `CodeGen/README.md`).
- `Preprocessor/` — Macro/conditional/include resolver and dependency graph emitter.
- `Lexer/`, `Parser/`, `AST/`, `Syntax/`, `Compiler/`, `Utils/` — Each phase has its own README describing responsibilities and APIs.

## Key entry points

- `include_resolver_load()` → `initLexer()` → `preprocessor_run()` → `initParser()` → `parse()` → `analyzeSemantics()`.
- Optional: build a `SemanticModel` via `analyzeSemanticsBuildModel()`, create a `CodegenContext` with `codegen_context_create("module", semanticModel)`, call `codegen_generate()`, then `LLVMDumpModule(codegen_get_module(ctx))`. Clean up with `codegen_context_destroy()` and `semanticModelDestroy()`.
