# Contributing to fisiCs

Thanks for contributing to `fisiCs`.

`fisiCs` is an experimental compiler under active development. Contributions are welcome, but changes must be small, test-backed, and low-risk.

## Workflow

1. For larger changes, open an issue first to align scope.
2. Submit changes via pull request.
3. Keep PRs focused on one logical change.
4. Include test evidence and update docs when behavior changes.

## Pull Request Requirements

- Maintainer approval is required before merge.
- Include exact commands used for verification.
- Include expected vs actual behavior for bug fixes.
- Update docs for user-visible behavior/interface changes.
- Avoid unrelated refactors in bug-fix PRs.

## Build and Test Baseline

Run at minimum:

```bash
make
make test
```

If your change affects final/binary behavior, also run relevant targets (examples):

```bash
make final
make test-binary
```

If full suites are too expensive, include the scoped subset you ran and explain why.

## Coding Expectations

- Preserve current build/test behavior unless your PR explicitly changes it.
- Prefer minimal, reviewable patches.
- Do not change licensing/attribution blocks in third-party or ambiguous-origin files.
- Keep naming and docs consistent with project conventions.

## Merge Policy

- `main` is maintainer-controlled.
- Preferred merge mode is squash merge for clean history.

