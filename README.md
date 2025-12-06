# Custom C Compiler

A lightweight, modular C-based compiler built from scratch, serving as the foundation for a future simulation-focused language with 
DSL-style annotations, physics-aware types, and IDE integration.

---

## Features (Current)

- **Lexer** – Robust tokenization for C99-style syntax (keywords, identifiers, literals, operators).
- **Preprocessor** – Runs before parsing: macro table + expansion engine (args, stringification, pasting, variadics), conditional stack for `#if/#elif/#else` with a dedicated PP expression evaluator, include resolver with project/system search paths, pragma-once/guard short-circuiting, and include-graph JSON emission (`--emit-deps-json` / `EMIT_DEPS_JSON`). `--preserve-pp` or `PRESERVE_PP_NODES=1` keeps directive AST stubs for tooling.
- **Parser** – Handles expressions, declarations, control-flow, designated initialisers, and attributes (`__attribute__`, `[[gnu::...]]`, `__declspec`, …). Declarators are normalized into `ParsedType` derivations (pointer chains, array suffixes, function signatures). GNU statement expressions can be toggled via `ENABLE_GNU_STATEMENT_EXPRESSIONS=1`; panic-mode recovery keeps parsing after errors so later diagnostics still appear.
- **Semantic Analysis** – Storage/linkage + tentative defs, deep pointer qualifiers, incomplete types, bitfields, integer const-eval (enums, case labels, array sizes, static initializers), switch diagnostics (duplicate cases, fallthrough warnings), varargs checks (default promotions, `va_start` validation), and strict layout inference (packed/aligned attributes, ABI profiles) feeding sizeof/alignof and codegen. Pointer arithmetic, lvalue/decay, and initializer-shape checks all replay the `ParsedType` derivations.
- **LLVM IR Codegen** – Ternary merge typing with PHI, short-circuiting, dense vs sparse switch lowering, pointer diff arithmetic, memcpy/memset for aggregates/zero-inits, and intptr width sourced from the LLVM module data layout. Types are rebuilt directly from `ParsedType` derivations; target triple and data layout propagate from CLI flags.
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
make tests      # includes parser/semantic/codegen + preprocessor regression scripts

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

Leave it unset (or `0`) to keep strict C99 behaviour—the parser will reject `({ ... })` constructs in that mode and emit a diagnostic explaining the extension is disabled.

### Preprocessing pipeline

- **Token provenance**: every token carries spelling location plus macro call-site/definition ranges; AST nodes inherit this for diagnostics.
- **Includes**: `INCLUDE_PATHS` (Makefile, defaults to `include`) controls search order. Quoted includes first try the including file’s directory, then project paths, then raw names; angle-bracket includes skip the including directory. Resolver caches file contents + mtimes, tracks `#pragma once` and classic guards, and records include edges for JSON emission.
- **Macros**: object- and function-like, variadic (`__VA_ARGS__`), stringification `#`, token-paste `##` (dangling edges handled), recursion guarded via expansion stack.
- **Conditionals**: dedicated evaluator for `#if/#elif` limited grammar, `defined(name)` support, unknown identifiers treated as `0`. Conditional stack suppresses inactive regions while still consuming directives.
- **Preservation**: pass `--preserve-pp` or `PRESERVE_PP_NODES=1` to keep lightweight directive nodes in the AST for tooling.
- **Dependency artifact**: emit include graph as JSON with `--emit-deps-json path` or `EMIT_DEPS_JSON=path` to feed build systems/IDEs.

### Preprocessor regression tests

Focused smoke tests live in `tests/preprocessor/`:

```bash
make preprocessor-tests
# or individually:
./tests/preprocessor/run_pp_if.sh ./compiler          # #if/#elif/#else stack
./tests/preprocessor/run_pp_stringify_paste.sh ./compiler
./tests/preprocessor/run_pp_variadic.sh ./compiler
./tests/preprocessor/run_pp_nested.sh ./compiler      # nested expansion
./tests/preprocessor/run_pp_include_basic.sh ./compiler
./tests/preprocessor/run_pp_include_search.sh ./compiler
./tests/preprocessor/run_pp_include_pragma_once.sh ./compiler
./tests/preprocessor/run_pp_preserve.sh ./compiler    # directive preservation
./tests/preprocessor/run_pp_deps.sh ./compiler        # dependency JSON emission
./tests/preprocessor/run_pp_expr.sh                   # pp_expr parser/evaluator
```

### Running Parser/Semantic/IR Smoke Tests

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
make recovery
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
