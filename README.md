# fisiCs Compiler

`fisiCs` is a C compiler project (C99 baseline, trending toward C17) with a layered, fail-closed test system and active external-program validation.

## Current Status

- Core compiler test suite: `make final` (latest local snapshot: `0 failing, 36 skipped`)
- Bucketed compiler coverage: lexer, preprocessor, parser, semantics, codegen, runtime, torture/differential
- Binary validation lane: active and expanded (SDL, ABI, linkage, stdio, math, corpus, clang differential)
- External validation milestones: `datalab` and `line_drawing` successfully compiled and run

## Build

Requirements:

- C compiler (`cc` / `clang`)
- LLVM toolchain (`llvm-config` available on `PATH`)
- `gperf`
- POSIX shell environment (macOS/Linux/WSL)

```bash
make
```

This builds:

- `fisics` (CLI compiler)
- `libfisics_frontend.a` (frontend static library)

## Quick Usage

```bash
# compile-only
./fisics -c hello.c -o hello.o

# compile + link
./fisics main.c util.c -o app

# include/lib paths
./fisics -Iinclude -L/path/to/lib -lm main.c -o app
```

Useful flags:

- `--dump-ast`
- `--dump-sema`
- `--dump-ir`
- `--dump-layout`
- `--target=<triple>`
- `--data-layout=<layout>`

## Testing

Primary entrypoints:

```bash
# compiler suite
make test
make final

# binary lanes
make test-binary
make test-binary-sdl
make test-binary-wave WAVE=<n> BINARY_WAVE_BUCKET=<bucket>
```

The harness is auto-discovery based, non-destructive by default, and designed to fail closed.

## Documentation

- Compiler/testing docs index: [`docs/00_docs_index.md`](docs/00_docs_index.md)
- Docs layout guide: [`docs/README.md`](docs/README.md)
- Test-system re-architecture context: [`docs/compiler_test_system_rearchitecture_context.md`](docs/compiler_test_system_rearchitecture_context.md)
- Coverage checklist: [`docs/compiler_behavior_coverage_checklist.md`](docs/compiler_behavior_coverage_checklist.md)
- Rolling execution log: [`docs/compiler_behavior_coverage_run_log.md`](docs/compiler_behavior_coverage_run_log.md)

## Repo Notes

- Build artifacts and scratch outputs are ignored (`build/`, `data/`, local binaries).
- `TOP_README.md` is retained as a legacy pointer; this `README.md` is now the canonical GitHub-facing summary.
