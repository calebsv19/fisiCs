# fisiCs Agent Guide

This repository contains `fisiCs`, an experimental C compiler with a
C99-oriented baseline, opt-in extension overlays, public examples, and a
CLI-first release workflow.

## Role

Use this repo to build, run, validate, and package the `fisiCs` compiler. The
public operating surface is intentionally CLI-first:

- compile C sources with `./fisics`
- run examples and canaries
- inspect diagnostics and build graph output
- run the public release-confidence checklist
- prepare local release archives when explicitly requested

## Start Here

1. Read `README.md`.
2. Read `docs/first_user_path.md`.
3. Read `docs/release_confidence_checklist.md`.
4. Read `docs/contributor_agent_quickstart.md`.
5. Use `docs/compiler_test_confidence_tiers.md` to choose validation scope.
6. Use `docs/cli_release_workflow.md` only when release packaging is in scope.
7. Use `examples/release_example_pack.md` for the smallest public demo path.
8. Use `examples/README.md` to choose broader example or curated-project workflows.

## Safe Routine Commands

Run from the repository root.

```bash
make
make test
make frontend-contract-test
make examples
make examples-canaries
./compilation/run_single.sh ./fisics
./compilation/run_multi.sh ./fisics
```

Use focused checks for changed compiler behavior:

```bash
make final-id ID=<test_id>
make final-bucket BUCKET=<bucket>
make final-manifest MANIFEST=<manifest>.json
```

Use these release-readiness checks only when packaging confidence is in scope:

```bash
make release-contract
make release-archive
make release-verify
make ci-guardrails
```

## Agent Workflow

1. Start with the first-user path before deeper tests.
2. Keep changes small and behavior-focused.
3. Run the narrowest validation that proves the touched surface.
4. Widen according to `docs/compiler_test_confidence_tiers.md`.
5. Include exact command evidence in the final report or PR.
6. Update public docs when behavior, commands, examples, or release workflow
   change.

## Boundaries

- Do not edit `VERSION`, build release artifacts, publish, deploy, or mutate
  production-registry state unless explicitly asked.
- Do not treat private CodeWork paths, VPS lanes, report-inbox tooling, or
  maintainer memory as public release requirements.
- Keep internal probe-worker orchestration and active failing-ledger work out
  of public docs unless a maintainer explicitly converts that workflow into a
  stable public contributor contract.
- Use `tests/final/probes/` only for frontier or unstable failure work; stable
  behavior should move back to the owning public validation lane.

## Release-Readiness Path

For a public release-readiness pass, use this order:

1. `make`
2. `examples/release_example_pack.md`
3. `make release-contract`
4. one changed-area final gate from `docs/release_confidence_checklist.md`
5. `make release-archive`
6. `make release-verify`

Signed/notarized distribution, bridge publishing, website metadata, and
production-registry promotion are separate maintainer approval lanes.
