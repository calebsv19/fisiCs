# Custom C Compiler

A lightweight, modular C-based compiler built from scratch, serving as the foundation for a future simulation-focused language with 
DSL-style annotations, physics-aware types, and IDE integration.

---

## Features (Current)

- **Lexer** – Robust tokenization for C99-style syntax, including all primitives, keywords, operators, and identifiers.  
- **Parser** – Stable parsing of expressions, types, control flow, and function declarations.  
- **H2 Generator** – Generates helper `.h2` metadata for code reference and IDE use.  
- **Designated Initializers** – Supports `{ .x = 1, .y = 2 }` struct initialization.  
- **Preprocessor Support** – Handles `#include`, `#define`, `#ifdef`, etc.  
- **Modular Architecture** – Clean separation of lexer, parser, header generation, and upcoming stages.

> **Not Yet Implemented:** Semantic analysis, LLVM backend/code generation.

---

## Screenshots / Diagrams

*(Add an image of a successful parse, pipeline diagram, or file structure overview here)*  
![Compiler Diagram](docs/compiler_pipeline.png)

---

## Getting Started

### Dependencies

- A C99-compliant compiler (`gcc` or `clang`)  
- POSIX-compatible system (Linux/macOS/WSL)  
- *(LLVM will be required later for backend codegen)*

### Build Instructions

```bash
git clone https://github.com/yourusername/custom-c-compiler.git
cd custom-c-compiler
make
./compiler path/to/your_file.c


--- or --

If you're not using make, here's the manual build line:
cc -o compiler src/*.c -Iinclude

### Running Parser Smoke Tests

The `tests/` folder holds focused parser fixtures (typedef chains, union declarations, designated initialisers, cast/sizeof initialisers).

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

Each script invokes `./compiler` against a minimal C snippet and asserts the expected AST fragments appear in the output. Extend these scripts whenever you add new syntactic features so regressions are caught early.
