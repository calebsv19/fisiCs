# Custom C Compiler

A lightweight, modular C-based compiler built from scratch, serving as the foundation for a future simulation-focused language with 
DSL-style annotations, physics-aware types, and IDE integration.

---

## Features (Current)

- **Lexer** – Robust tokenization for C99-style syntax (keywords, identifiers, literals, operators).  
- **Parser** – Handles expressions, declarations, control-flow, designated initialisers, function pointers, and (optionally) GNU statement expressions when `ENABLE_GNU_STATEMENT_EXPRESSIONS=1` is set in the environment.  
- **Semantic Analysis** – Tracks typedef/tag namespaces, enforces lvalue/decay rules, qualifier-safe assignments, pointer arithmetic semantics, records function prototypes (including variadic signatures) and tag-layout fingerprints, validates initializer shapes, understands block-scope variable-length arrays, and treats compound literals as lvalues within their lifetime while flagging illegal static-storage uses.  
- **LLVM IR Codegen** – Lowers analysed ASTs into LLVM IR (struct/union layouts, arrays including VLAs, compound assignments, loops/conditionals, globals).  
- **Preprocessor Support** – Parses `#include`, `#define`, `#ifdef`, etc., preserving directives in the AST pipeline.  
- **H2 Generator** – Emits helper `.h2` metadata for IDE or tooling experiments.  
- **Debug Instrumentation** – Opt-in parser/codegen tracing toggled at build time via make variables.

---

## Screenshots / Diagrams

*(Add an image of a successful parse, pipeline diagram, or file structure overview here)*  
![Compiler Diagram](docs/compiler_pipeline.png)

---

## Getting Started

### Dependencies

- A C99-compliant compiler (`gcc` or `clang`)  
- LLVM toolchain (tested with Homebrew LLVM 21) for headers/libs used by IR generation  
- POSIX-compatible system (Linux, macOS, or WSL)

### Build Instructions

```bash
git clone https://github.com/yourusername/custom-c-compiler.git
cd custom-c-compiler

# build + unit smoke suite
make
make tests

# run built-in demo (parse → semantics → LLVM IR dump)
make run
```

Optional debug toggles exposed via the makefile:

```bash
# parser diagnostics
make PARSER_DEBUG=1 run

# codegen tracing / debug
make CODEGEN_TRACE=1 run
make CODEGEN_DEBUG=1 run

# mix flags as needed
make PARSER_DEBUG=1 CODEGEN_TRACE=1 run
```

Manual build shortcut:

```bash
cc -o compiler src/*.c -Iinclude $(llvm-config --cflags --ldflags --libs core)
```

### GNU Statement Expressions (optional)

Parsing/semantic support for GNU `({ ... })` statement expressions is behind the `ENABLE_GNU_STATEMENT_EXPRESSIONS` environment flag. Set it to a non-empty value when running the compiler or spec harness to allow them:

```bash
ENABLE_GNU_STATEMENT_EXPRESSIONS=1 ./compiler tests/parser/gnu_statement_expr.c
```

Leave it unset (or `0`) to keep strict C99 behaviour—the parser will reject `({ ... })` constructs in that mode.

### Running Parser Smoke Tests

The `tests/` folder holds focused fixtures covering parser, semantic, and LLVM IR behaviour (typedef chains, union declarations, designated initialisers, semantic diagnostics, etc.).

```bash
# run them all
make tests

# run AST/diagnostic spec assertions (golden files)
make spec-tests

# or individually
make union-decl
make initializer-expr
make typedef-chain
make designated-init
make control-flow
make cast-grouped
make for-typedef
make function-pointer
make pointer-arith
make switch-flow
make goto-flow
make statement-expr-enabled
make statement-expr-disabled
make semantic-typedef
make semantic-initializer
make semantic-undeclared
make semantic-bool
make semantic-invalid-arith
make semantic-lvalue-errors
make semantic-pointer-errors
make semantic-pointer-qualifier
make semantic-function-calls
make semantic-tag-conflicts
make semantic-initializer-shapes
make semantic-flow
make semantic-vla-errors
make semantic-vla-block
make compound-literal-lvalues

# refresh goldens after intentional output changes
UPDATE_GOLDENS=1 tests/spec/run_ast_golden.sh ./compiler
```

Each script invokes `./compiler` against a minimal snippet and asserts on AST fragments, semantic diagnostics, or IR output. Extend the suite whenever you add new surface area so regressions are caught early.
