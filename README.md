# Custom C Compiler

A lightweight, modular C-based compiler built from scratch, serving as the foundation for a future simulation-focused language with 
DSL-style annotations, physics-aware types, and IDE integration.

---

## Features (Current)

- **Lexer** – Robust tokenization for C99-style syntax (keywords, identifiers, literals, operators).  
- **Parser** – Handles expressions, declarations, control-flow, designated initialisers, and function pointers (Pratt or legacy ladders).  
- **Semantic Analysis** – Builds scope graphs, tracks typedef/tag namespaces, performs basic type compatibility, and emits aggregated diagnostics.  
- **LLVM IR Codegen** – Lowers analysed ASTs into LLVM IR (struct/union layouts, arrays, compound assignments, loops/conditionals, globals).  
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

### Running Parser Smoke Tests

The `tests/` folder holds focused fixtures covering parser, semantic, and LLVM IR behaviour (typedef chains, union declarations, designated initialisers, semantic diagnostics, etc.).

```bash
# run them all
make tests

# or individually
make union-decl
make initializer-expr
make typedef-chain
make designated-init
make control-flow
make cast-grouped
make for-typedef
make function-pointer
make semantic-typedef
make semantic-initializer
make semantic-undeclared
make semantic-bool
```

Each script invokes `./compiler` against a minimal snippet and asserts on AST fragments, semantic diagnostics, or IR output. Extend the suite whenever you add new surface area so regressions are caught early.
