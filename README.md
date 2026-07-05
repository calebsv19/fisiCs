# fisiCs Compiler

`fisiCs` is an experimental C compiler project with a C99-oriented baseline,
active hardening toward broader C17-compatible behavior, a fail-closed test
system, and real-project validation.

The current public contract is "default C first, extensions opt-in." The
project priority is correctness, stability, and reproducible behavior on real
programs before broader language or tooling ambitions.

## Start Here

- Download the current packaged compiler:
  [`ecosystem.calebsv.tech/suite/program/?repo=fisiCs`](https://ecosystem.calebsv.tech/suite/program/?repo=fisiCs)
- Agent-discoverable release entrypoint:
  [`ecosystem.calebsv.tech/agents/START_HERE.md`](https://ecosystem.calebsv.tech/agents/START_HERE.md)
- Current support boundary:
  [`docs/supported_feature_matrix.md`](docs/supported_feature_matrix.md)
- Shortest successful user path:
  [`docs/first_user_path.md`](docs/first_user_path.md)
- Agent operating guide:
  [`AGENTS.md`](AGENTS.md)
- Smaller release-ready checkpoint flow:
  [`docs/release_confidence_checklist.md`](docs/release_confidence_checklist.md)
- Overlay contract:
  [`docs/extension_overlays.md`](docs/extension_overlays.md)

## Current Status

- Active development; not yet a finished production compiler.
- Default mode remains ordinary C compilation.
- Core compiler trust lanes cover lexer, preprocessor, parser, semantics,
  codegen, runtime, and differential surfaces.
- Host-available standard-header runtime/header coverage is green through Wave
  `323` on the current validation host.
- Real-project validation currently records Stages `A` through `F` closed for
  the current validated project set, with no active open risk in the saved
  reports.
- `clang` remains a baseline reference compiler while `fisiCs` continues hardening.
- The extension-overlay framework is live behind explicit opt-in flags; the
  first public lane is physics-units metadata/checking via
  `[[fisics::dim(...)]]`.

## Build

Use this path when you want to build the compiler from source. If you only
want to try the current macOS Apple Silicon package, use the download link in
the Start Here section and follow `docs/first_user_path.md`.

Requirements:

- C compiler (`cc` / `clang`)
- LLVM toolchain (`llvm-config` available on `PATH`)
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
- Some surfaces are still expanding and do not yet claim full diagnostics
  parity.
- The user-facing support boundary is documented in
  [`docs/supported_feature_matrix.md`](docs/supported_feature_matrix.md).
- Development is quality-first and test-driven; broad feature additions remain
  secondary to compiler reliability.

## Quick Usage

```bash
# metadata
./fisics --version
./fisics --help

# compile-only
./fisics -c examples/hello_world.c -o /tmp/fisics_hello_world.o

# compile + link
./fisics compilation/multi_main.c compilation/multi_helper.c -o /tmp/fisics_multi_bin
/tmp/fisics_multi_bin

# include/lib paths
./fisics -Iinclude -L/path/to/lib -lm main.c -o app

# source-level build graph JSON
./fisics --emit-build-graph-json graph.json -Iinclude main.c

# local manifest dry-run build graph JSON
./fisics --build-manifest project.json --dry-run --json \
  --emit-build-graph-json graph.json

# local manifest compile database export
./fisics --build-manifest project.json --emit-compile-db compile_commands.json

# local manifest build execution
./fisics --build-manifest project.json
```

Build graph JSON includes a compact `diagnostic_summary` object at the graph
and translation-unit levels. It carries counts and `partial` / `fatal` state;
full diagnostic payloads stay in `--emit-diags-json` / `--emit-diags-pack`.

Useful flags:

- `--version`
- `--help`
- `--dump-ast`
- `--dump-sema`
- `--dump-ir`
- `--dump-layout`
- `--emit-build-graph-json <path>`
- `--build-manifest <path> --dry-run --json`
- `--emit-compile-db <path>` with `--build-manifest`
- `--target=<triple>`
- `--data-layout=<layout>`

Overlay-specific flags:

- `--overlay=physics-units`
- `--overlay=memory-check`
- `--overlay=ide-metadata`
- `--overlay=all`

`memory-check` is explicit opt-in allocation/free lifecycle diagnostics and is
not included in `--overlay=all`.

The current overlay model is documented in [`docs/extension_overlays.md`](docs/extension_overlays.md).

For the clean-user path from clone to first successful examples, use
[`docs/first_user_path.md`](docs/first_user_path.md).

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
make memory-check-test

# public CLI metadata contract
make integration-cli-metadata

# explicit local historical fixture smoke
make dev-smoke

# focused probe lane (outside make final)
python3 tests/final/probes/run_probes.py

# binary lanes
make test-binary
make test-binary-sdl
make test-binary-wave WAVE=<n> BINARY_WAVE_BUCKET=<bucket>

# real-project lane
make realproj-stage-a REAL_PROJECT=<project>
make realproj-stage-b REAL_PROJECT=<project>
make realproj-stage-c REAL_PROJECT=<project>
make realproj-stage-d REAL_PROJECT=<project>
make realproj-stage-e REAL_PROJECT=<project>
make realproj-stage-f REAL_PROJECT=<project>
```

The harness is auto-discovery based, non-destructive by default, and designed to fail closed.

For the smaller release/user-facing checkpoint flow, use
[`docs/release_confidence_checklist.md`](docs/release_confidence_checklist.md).

## Repository Layout

- `src/`: compiler implementation (lexer, parser, semantics, codegen, frontend)
- `include/`: local include fixtures and test-support headers
- `tests/`: harnesses, suites, final/binary manifests, expectations
- `examples/`: runnable examples and practical canaries (`hello_world`,
  multi-TU/libc/math canaries, SDL loop, physics-units pilot, opt-in
  memory-check leak demo)
- `docs/`: public project docs and validation references
- `compilation/`: quick compile/link examples and scripts
- `makefile`: build and test entrypoints

## Documentation

- Compiler/testing docs index: [`docs/00_docs_index.md`](docs/00_docs_index.md)
- Docs layout guide: [`docs/README.md`](docs/README.md)
- Supported feature matrix: [`docs/supported_feature_matrix.md`](docs/supported_feature_matrix.md)
- First user path: [`docs/first_user_path.md`](docs/first_user_path.md)
- Release confidence checklist: [`docs/release_confidence_checklist.md`](docs/release_confidence_checklist.md)
- Agent operating guide: [`AGENTS.md`](AGENTS.md)
- Contributor/agent quickstart: [`docs/contributor_agent_quickstart.md`](docs/contributor_agent_quickstart.md)
- Public roadmap: [`docs/public_roadmap.md`](docs/public_roadmap.md)
- Examples quickstart: [`examples/README.md`](examples/README.md)
- Release example pack: [`examples/release_example_pack.md`](examples/release_example_pack.md)
- Practical public canaries: [`examples/canaries/README.md`](examples/canaries/README.md)
- Physics-units pilot example: [`examples/physics_units/README.md`](examples/physics_units/README.md)
- Test-system re-architecture context: [`docs/compiler_test_system_rearchitecture_context.md`](docs/compiler_test_system_rearchitecture_context.md)
- Test workflow guide: [`docs/compiler_test_workflow_guide.md`](docs/compiler_test_workflow_guide.md)
- Full validation workflow: [`docs/validation_workflow.md`](docs/validation_workflow.md)
- Compiler/IDE data contract: [`docs/compiler_ide_data_contract.md`](docs/compiler_ide_data_contract.md)
- Extension overlays and physics-units lane: [`docs/extension_overlays.md`](docs/extension_overlays.md)
- CLI release workflow: [`docs/cli_release_workflow.md`](docs/cli_release_workflow.md)
- Real-project validation scaffold: [`tests/real_projects/README.md`](tests/real_projects/README.md)

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
