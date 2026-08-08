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
- OpenAI Build Week judge install and test path:
  [`docs/build_week_judge_guide.md`](docs/build_week_judge_guide.md)
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
- The compiler-owned OS Policy lane is closed through OS-P3 stress and its
  immutable post-EDU-19 policy-contract intake now covers EDU-21 Wire-v1 and
  EDU-22 Queue/Trace-v1, EDU-23 bounded parallelism through EDU-31
  monotonic-time arithmetic, EDU-32 Workload-v1 through EDU-38 Wire-v13 and
  bounded runner contexts, EDU-39 phase ownership, EDU-40 AP-mailbox ownership,
  and EDU-41 two-active cooperative-runner selection. Composition probes now
  join EDU-40/41 for wrong-owner rejection and peer-preserving retirement, then
  join EDU-26/35/37/39/40/41 into one fail-closed durable owner chain from
  generation-safe reuse through checkpoint, phase, runner, and mailbox
  dispatch identity. Temporal fault-sequence and paired temporal-contradiction
  probes now exercise pre-ACK interruption, restart identity, retirement
  ordering, stale evidence, and peer preservation. Four reduced deterministic
  temporal oracles are promoted in torture-differential wave `140`. Wave
  `141` adds a reduced cross-model temporal oracle joining Queue-v2, Wire-v7,
  scheduler handoff, and stale-evidence rejection. Wave `142` adds a reduced
  source-derived frozen-program-selection oracle for the real EDU-48 bundle
  contract; signing, artifact persistence, and guest loading remain outside
  that C proof. The broad temporal composition matrix remains explicitly
  probe-only. The promotion audit currently records `3578` mapped promotions, `22` intentional probe-only
  entries, no missing candidates, and no critical or ambiguous integrity
  findings. The stable final suite contains `4990` tests; its latest monitored
  checkpoint closes at `4990` passes, `0` failures, and `36` expected skips.
- Host-available standard-header runtime/header coverage is green through Wave
  `323` on the current validation host.
- Real-project validation currently records Stages `A` through `F` closed for
  the current validated project set, with no active open risk in the saved
  reports.
- `clang` remains a baseline reference compiler while `fisiCs` continues hardening.
- The extension-overlay framework is live behind explicit opt-in flags; the
  first public lane is physics-units metadata/checking via
  `[[fisics::dim(...)]]`.

## OpenAI Build Week 2026

`fisiCs` is a pre-existing project being meaningfully extended during the
OpenAI Build Week submission period (July 13-21, 2026). The frozen pre-period
baseline is commit
[`aa4b3268ce552fd3ead88fa9b6bd8df52842b3df`](https://github.com/calebsv19/fisiCs/commit/aa4b3268ce552fd3ead88fa9b6bd8df52842b3df).

The Build Week candidate focuses on turning the compiler's existing breadth
into a judge-testable developer tool:

- production-shaped differential validation against real CodeWork programs
- compiler fixes found through those real-project workflows
- deterministic example projects covering normal compilation, multi-file
  builds, physics-units diagnostics, and build-graph output
- a clean install, package verification, and short public test path

The first public submission candidate was version `0.3.0`, ending at the
immutable `v0.3.0` release tag. The submission-facing stabilization line now
continues through commit
[`3256df3555af09772a41079dd9357ac0120e7ba2`](https://github.com/calebsv19/fisiCs/commit/3256df3555af09772a41079dd9357ac0120e7ba2)
and is released separately as `0.4.0` so the published `0.3.0` source and
artifact identities remain unchanged. The principal Codex `/feedback` Session
ID is `019f6486-6fc5-79b2-91dd-4ecab0b34118`.

The recorded Build Week compiler demonstration uses that immutable `0.4.0`
package. The current release line advances to `0.5.0` with explicit
freestanding `x86_64-unknown-none` ELF64 object emission. Archive names and
SHA-256 values are version-specific, so use the current program page or the
matching GitHub Release rather than copying the filename or checksum shown in
the recorded demonstration.

### How Codex and GPT-5.6 contributed

The project was developed iteratively with Codex and GPT-5.6. In the principal
session, GPT-5.6 helped turn the existing real-project harness into a reusable
Stage-G operational-differential system, then used it across DAW, the CodeWork
IDE, and `fisiCs` itself. That work isolated compiler defects from stale
goldens and fixture-oracle problems, produced focused compiler repairs and
permanent regressions, and verified repeated Clang-versus-`fisiCs` trace and
artifact parity instead of treating matching exit codes as sufficient proof.

The final `fisiCs` checkpoint closes four repeated Stage-G workflows at
`4 both_pass / 0 blockers`, records `3478` promoted tests with no missing,
critical, or ambiguous entries, and completes the monitored suite at `4893`
passes, `0` failures, and `36` expected skips across `4929` outcomes. Codex
also assisted with repository inspection, change grouping, release planning,
package/readback verification, and documentation reconciliation. The
maintainer selected the product direction, compatibility boundaries,
extension policy, acceptance criteria, release authority, and which generated
changes were kept or rejected.

For the shortest judge-facing path, use the
[Build Week judge guide](docs/build_week_judge_guide.md), followed by the
[release example pack](examples/release_example_pack.md). They demonstrate the
compiler without requiring the full internal validation suite.

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

The explicit `--target x86_64-unknown-none` compile-only lane emits
freestanding System V ELF64 x86-64 relocatable objects on supported build
hosts, including Apple Silicon. It selects the x86-64 backend directly, uses
static relocations and the small code model, derives the data layout from that
target machine, and marks every function `noredzone`; it does not reuse the
host's Mach-O format or native target defaults. The focused object-contract
gate is:

```bash
make integration-x86_64-freestanding-object
```

The compiler-owned OS Policy lane composes that target contract with bounded
hardware-blind policy semantics:

```bash
make os-policy-object
make os-policy-runtime
make os-policy-guest
make os-policy
```

See [`docs/os_policy_validation.md`](docs/os_policy_validation.md). OS-P1 now
proves repeated fisiCs-versus-Clang execution in a shared minimal QEMU guest,
and OS-P2 now includes storage/result, a shared 31-vector ELF field-admission
corpus, a shared 27-vector job-admission corpus, and a shared 44-vector raw
queue-record/transition corpus, plus a shared 60-vector stateful scheduler
transition corpus and a shared 51-assertion synchronization/rank corpus. This
does not claim downstream `os-dev` kernel, complete ELF-loader, durable queue
storage, assembly-owned atomics/context switching, interrupt delivery, lock
fairness, or real-hardware correctness.

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

For Decision-bound release-control runs, sandbox keychain output is
non-authoritative. A sandbox result of `0 valid identities found` routes to the
host `codework-apple-release` named-profile status check; it does not prove that
the Developer ID credential is absent. Strict signature verification is also a
host-stage operation; a sandbox-only verification failure cannot become a
durable artifact blocker. See
[`docs/cli_release_workflow.md`](docs/cli_release_workflow.md).

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

# freestanding OS-policy lane
make os-policy

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
- Build Week judge guide: [`docs/build_week_judge_guide.md`](docs/build_week_judge_guide.md)
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
