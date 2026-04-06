# Contributor and Agent Quickstart

This file is the fastest entrypoint for both human contributors and Codex-style agents.

## 1) Read First

1. `README.md` (project scope, maturity, build/test basics)
2. `docs/public_roadmap.md` (current priorities and direction)
3. `docs/compiler_test_workflow_guide.md` (test-work operating flow)

## 2) Build and Verify

```bash
make
make test
```

When relevant to your change:

```bash
make final
make test-binary
```

## 3) Contribution Expectations

- Keep changes small and focused.
- Include exact test evidence in PRs.
- Update docs when behavior/interfaces/workflows change.
- Use `.github/CONTRIBUTING.md` as the merge/readiness contract.

## 4) Docs Boundary

- Public docs stay in `fisiCs/docs/`.
- Internal planning, deep run logs, and private triage stay in:
  - `docs/private_program_docs/fisiCs/`

## 5) Troubleshooting Orientation

- Compiler architecture and test-system constraints:
  - `docs/compiler_test_system_rearchitecture_context.md`
  - `docs/compiler_test_coverage_blueprint.md`
- Frontend embedding/API integration:
  - `docs/frontend_api.md`
- Full-project compile-validation flow:
  - `docs/validation_workflow.md`

