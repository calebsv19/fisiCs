# Docs Index

This is the table of contents for the public `docs/` tree in `fisiCs`.
Last audited: 2026-07-23.

## Start Here

1. `README.md`
2. `AGENTS.md`
3. `docs/supported_feature_matrix.md`
4. `docs/first_user_path.md`
5. `docs/build_week_judge_guide.md`
6. `docs/release_confidence_checklist.md`
7. `docs/contributor_agent_quickstart.md`
8. `docs/public_roadmap.md`
9. `examples/README.md`
10. `examples/release_example_pack.md`
11. `examples/canaries/README.md`
12. `examples/physics_units/README.md`

## Public Docs In `docs/`

### `docs/00_docs_index.md`

Primary table of contents for the public docs lane.

### `docs/README.md`

Public docs layout summary and private-boundary policy.

### `docs/supported_feature_matrix.md`

Current public support boundary for baseline C, header/runtime scope,
compile/link behavior, build-graph/local-manifest dry-run tooling scope,
behavior policy, and opt-in extension boundaries.

### `docs/first_user_path.md`

Shortest supported user path from build through hello world, multi-TU smoke,
physics-units pilot, and minimum smoke validation.

### `docs/build_week_judge_guide.md`

OpenAI Build Week installation, supported-platform, no-rebuild testing,
submission evidence, and `/feedback` Session ID reference.

### `docs/release_confidence_checklist.md`

Smaller contributor-facing release-readiness command ladder.
Memory-check overlay changes use `make memory-check-test` before broader
runtime-surface gates.

### `docs/contributor_agent_quickstart.md`

Fast operating flow for contributors and automated tooling.

### `docs/public_roadmap.md`

High-level roadmap and current priorities.

### `docs/compiler_test_system_rearchitecture_context.md`

Core constraints and end-state targets for compiler test architecture.

### `docs/compiler_test_architecture.md`

Authoritative public map of the test-system structure, confidence layers, suite roles, and test-routing model.

### `docs/compiler_test_failure_taxonomy.md`

Authoritative public vocabulary for failure kinds, severity, origin, and lane mapping across compiler test suites.

### `docs/compiler_test_regression_intake.md`

Canonical public workflow for taking a failure found in a real program or canary lane and turning it into permanent regression coverage.

### `docs/compiler_test_confidence_tiers.md`

Canonical public command ladder for choosing the right confidence tier, from build-only sanity through full trust and timing checkpoints.

### `docs/compiler_test_coverage_blueprint.md`

Coverage architecture and harness organization plan.

### `docs/compiler_test_workflow_guide.md`

Operational guide for bucket-by-bucket compiler test execution.

### `docs/os_policy_validation.md`

Compiler-owned OS Policy (`OS-P`) contract covering deterministic host
differential semantics, repeated `x86_64-unknown-none` object invariants,
repeated fisiCs-versus-Clang QEMU guest proof, bounded policy-family expansion,
and downstream `os-dev` canary boundaries.

### `docs/validation_workflow.md`

Public workflow for the real-project validation ladder, including compile,
runtime, and timing-oracle roles.

### `docs/frontend_api.md`

Reference notes for the reusable frontend API/library flow.

### `docs/extension_overlays.md`

Public reference for the opt-in extension framework and the current
physics-units and memory-check overlay surfaces, including canonical unit
naming, explicit conversion boundaries, memory-check runtime diagnostics
limits, and widened unit-family coverage.

### `docs/compiler_ide_data_contract.md`

Versioned compiler-to-IDE communication contract (schema, stability,
compatibility), including current additive diagnostics metadata and explanation
surfaces.

### `docs/cli_release_workflow.md`

CLI release packaging/sign/notarize workflow for macOS distribution artifacts.

### `docs/make_final_timing_log.md`

Public macro-trend timing lane for `make final`, including capture workflow and CSV metrics contract.

## Private Docs

Internal planning, status logs, and deep execution artifacts are intentionally maintained in private maintainer documentation outside this public docs lane.
