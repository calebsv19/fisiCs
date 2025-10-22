# Source Overview

This tree implements a single-pass C front-end with optional LLVM IR emission. Control flows through the classic phases:

- **Driver** (`main.c`) loads test source via `Utils`, configures the shared `CompilerContext`, then executes lexing → parsing → semantic analysis (and optionally code generation/AST dumps).
- **Lexer** tokenises the raw buffer into `Token` objects, including keyword lookup via an auto-generated gperf table.
- **Parser** consumes tokens into an annotated AST. Pratt and recursive-descent expression engines coexist, sharing helper utilities for type probing, declarations, and designators.
- **AST** defines the node model plus printers for debugging.
- **Syntax** runs semantic checks: symbol binding, scope management, and basic type validation.
- **Compiler** tracks typedef/tag namespaces so the parser and semantic passes can distinguish user types from identifiers.
- **CodeGen** (experimental) lowers AST nodes into LLVM IR blocks.


## Files in this directory

- `main.c` — Hard-wired demo pipeline (`parse()` + `analyzeSemantics()` + optional LLVM build); toggles let you print tokens/AST or emit IR.

## Subdirectories of note

- `CodeGen/` — LLVM IR backend (see `CodeGen/README.md` for emitter details).
- `Lexer/`, `Parser/`, `AST/`, `Syntax/`, `Compiler/`, `Utils/` — Each phase exposes its own README describing responsibilities and APIs.


## Key entry points

- `readFile()` (Utils) → `initLexer()` (Lexer) → `initParser()` (Parser/Helpers) → `parse()` (Parser) → `analyzeSemantics()` (Syntax).
- Optional: build a `SemanticModel` via `analyzeSemanticsBuildModel()`, create a `CodegenContext` with `codegen_context_create("module", semanticModel)`, call `codegen_generate()` to populate the module, then `LLVMDumpModule(codegen_get_module(ctx))` for inspection. Clean up with `codegen_context_destroy()` and `semanticModelDestroy()`.
