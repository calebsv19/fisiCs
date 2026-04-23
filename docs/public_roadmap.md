# fisiCs Public Roadmap

Last updated: 2026-04-22.

## Current Focus

- Maintain compiler correctness and stability on real project workloads.
- Expand and harden test coverage across parser, semantics, codegen, and runtime behavior.
- Continue external-program validation while preserving deterministic, reproducible diagnostics.
- Drive higher-stress validation campaigns across runtime, linkage, ABI, and diagnostics parity surfaces.

## Near-Term Priorities

- Improve diagnostics quality and failure-mode clarity.
- Reduce known edge-case gaps identified by regression and probe backlogs.
- Keep binary validation lanes healthy (`SDL`, `ABI`, `linkage`, `stdio`, `math`, differential).
- Promote newly validated stress scenarios into stable final-manifest coverage.

## Medium-Term Direction

- Increase confidence for daily internal use of `fisiCs` as a serious compiler toolchain.
- Continue quality/performance hardening before broad language-surface expansion.

## Long-Term Direction

- After core maturity, extend the language/model surface toward physics-oriented metadata and tighter IDE-integrated analysis workflows.

## Maturity Note

`fisiCs` is under active development and remains experimental.
`clang` is still used as a reference/baseline compiler in validation workflows.
