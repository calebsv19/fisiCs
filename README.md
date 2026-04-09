# fisiCs Compiler

`fisiCs` is an experimental C compiler project (C99 baseline, trending toward C17) with a layered, fail-closed test system and active external-program validation.

The current priority is compiler correctness, stability, and reproducible behavior on real programs. After core compiler maturity is stronger, the project direction expands toward physics-oriented language metadata and stricter IDE-integrated analysis workflows.

## Current Status

- Active development; not yet a finished production compiler.
- Core compiler test suite: `make final` (recent local snapshot: `0 failing, 36 skipped`)
- Bucketed compiler coverage: lexer, preprocessor, parser, semantics, codegen, runtime, torture/differential
- Binary validation lane: active and expanded (SDL, ABI, linkage, stdio, math, corpus, clang differential)
- External validation milestones: `datalab` and `line_drawing` successfully compiled and run
- `clang` remains a baseline reference compiler while `fisiCs` continues hardening.

## Build

Requirements:

- C compiler (`cc` / `clang`)
- LLVM toolchain (`llvm-config` available on `PATH`)
- `gperf`
- POSIX shell environment (macOS/Linux/WSL)
- Vendored shared subtree at `third_party/codework_shared/` (sync via `../bin/update_shared_subtrees.sh --update --only fisiCs --targets ../bin/shared_subtree_targets.tsv`)

```bash
make
```

This builds:

- `fisics` (CLI compiler)
- `libfisics_frontend.a` (frontend static library)

## Limitations and Maturity

- Experimental project: expect rough edges and active behavior changes.
- Some surfaces are still expanding and may have incomplete diagnostics parity.
- Development is quality-first and test-driven; broad feature additions are secondary to compiler reliability.

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

## CLI Release Packaging (macOS)

`fisiCs` now includes a CLI-first release lane (no `.app` required):

```bash
make release-contract
make release-archive
make release-verify
```

For signed/notarized distribution:

```bash
make release-sign APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)"
make release-notarize APPLE_SIGN_IDENTITY="Developer ID Application: <Name> (<TEAMID>)" APPLE_NOTARY_PROFILE="<profile>"
```

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

## Repository Layout

- `src/`: compiler implementation (lexer, parser, semantics, codegen, frontend)
- `include/`: local include fixtures and test-support headers
- `tests/`: harnesses, suites, final/binary manifests, expectations
- `examples/`: minimal runnable examples (`hello_world`, SDL loop)
- `docs/`: public project docs and validation references
- `compilation/`: quick compile/link examples and scripts
- `makefile`: build and test entrypoints

## Documentation

- Compiler/testing docs index: [`docs/00_docs_index.md`](docs/00_docs_index.md)
- Docs layout guide: [`docs/README.md`](docs/README.md)
- Contributor/agent quickstart: [`docs/contributor_agent_quickstart.md`](docs/contributor_agent_quickstart.md)
- Public roadmap: [`docs/public_roadmap.md`](docs/public_roadmap.md)
- Examples quickstart: [`examples/README.md`](examples/README.md)
- Test-system re-architecture context: [`docs/compiler_test_system_rearchitecture_context.md`](docs/compiler_test_system_rearchitecture_context.md)
- Test workflow guide: [`docs/compiler_test_workflow_guide.md`](docs/compiler_test_workflow_guide.md)
- Full validation workflow: [`docs/validation_workflow.md`](docs/validation_workflow.md)
- Compiler/IDE data contract: [`docs/compiler_ide_data_contract.md`](docs/compiler_ide_data_contract.md)
- CLI release workflow: [`docs/cli_release_workflow.md`](docs/cli_release_workflow.md)

## Contributing

Contributions are welcome, especially focused bug fixes and test-backed improvements.

Please read:

- [`.github/CONTRIBUTING.md`](.github/CONTRIBUTING.md)
- [`.github/CODE_OF_CONDUCT.md`](.github/CODE_OF_CONDUCT.md)
- [`.github/SECURITY.md`](.github/SECURITY.md)

PR expectations:

- Keep changes small and reviewable.
- Include exact test evidence.
- Update docs when behavior/interfaces change.

## License

This project is licensed under the Apache License 2.0.
See [`LICENSE`](LICENSE).

## Repo Notes

- Build artifacts and scratch outputs are ignored (`build/`, `data/`, local binaries).
- `TOP_README.md` is retained as a legacy pointer; this `README.md` is now the canonical GitHub-facing summary.
