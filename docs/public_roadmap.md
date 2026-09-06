# fisiCs Public Roadmap

Last updated: 2026-09-03.

## Current Focus

- Maintain compiler correctness and stability on real project workloads.
- Expand and harden test coverage across parser, semantics, codegen, and runtime behavior.
- Continue external-program validation while preserving deterministic,
  reproducible diagnostics.
- Keep the completed IDE analysis bridge ingestion/publication boundary stable
  while IDE UI work plans how to surface the new diagnostics, units, build
  graph, include graph, and memory-report data.

## Near-Term Priorities

- Keep the additive diagnostics contract stable while expanding consumer usage.
- Reduce known edge-case gaps identified by regression and probe backlogs.
- Keep binary validation lanes healthy (`SDL`, `ABI`, `linkage`, `stdio`, `math`, differential).
- Promote newly validated stress scenarios into stable final-manifest coverage.
- Harden the new opt-in extension overlay surface without weakening core C compiler behavior.
- Add and maintain a small public physics-units pilot example so users and
  coding agents have one canonical overlay usage reference.
- Keep widening the concrete-unit registry in bounded family batches while
  preserving explicit conversion-only policy and mechanical registry validation.
- Preserve the implemented `semantic-metadata` identity/domain/role and
  `produces`/`consumes` relationship contracts plus the additive extended
  frontend and translation-unit JSON export. Keep the strict semantic project
  graph stable without resizing the legacy
  caller-allocated frontend result or changing build-manifest v0.
- Keep the implemented physical-composition contract stable: independent
  quantity meaning, dimensions, units, value form, reference semantics, and
  frames compose into scoped evidence without becoming a hardware- or
  simulation-specific unit lane.
- Keep the implemented static hardware compatibility contract stable: one
  bounded canary over reusable component catalogs, configured topology,
  exact driver/model joins,
  allocation conflicts, and physical signal bindings, with every runtime and
  physical state remaining explicitly unassessed.
- Maintain the public extension quickstart, portable Clang/fisiCs header, and
  runnable semantic-metadata examples as the adoption surface for the complete
  static graph pipeline before designing runtime evidence or simulation binding.
- Maintain the repository-owned extension agent contract and its specialized
  units plus companion routing skills. The public in-repository examples now
  use portability macros under strict Clang and fisiCs semantic-parity gates;
  migrate external customers only through separately scoped adoption passes.

## Medium-Term Direction

- Increase confidence for daily internal use of `fisiCs` as a serious compiler toolchain.
- Continue quality/performance hardening before broad language-surface expansion.
- Extend the new overlay framework through physics-units semantics, broader
  practical unit coverage, and IDE-consumable metadata while keeping those
  features explicitly opt-in.
- Extend the established typed semantic attachment substrate through an
  implemented internal relationship graph, additive translation-unit export,
  strict project assembly, general physical composition, and one bounded
  hardware-compatibility proof before any simulation/runtime consumer. The
  contracts and historical source audit live in
  `docs/semantic_metadata_contract.md` and
  `docs/semantic_metadata_implementation_plan.md`; the semantic project graph
  contract is in `docs/semantic_metadata_project_graph_contract.md`, the
  physical composition contract is in
  `docs/semantic_metadata_physical_composition_contract.md`, and the hardware
  compatibility contract is in
  `docs/semantic_metadata_hardware_contract.md`.
- Use build graph diagnostic summaries, diagnostic explanations,
  include/macro context, units details, and memory-check report sidecars as
  inputs to IDE UI/presentation work rather than expanding the compiler ABI.

## Metadata Adoption Sequence

Future metadata work should be driven by real programs rather than another
abstract compiler layer:

1. Model one small, real static hardware project using current declarations,
   units evidence, physical contracts, component catalogs, and topology.
2. Record any meaning that the pilot cannot express without awkward or unsafe
   workarounds.
3. Add callable or data-carrier bindings for parameters, return values, or
   members only when that gap report demonstrates a concrete need.
4. Refine reusable device catalogs with exact model/revision and source
   provenance while keeping catalog claims separate from physical presence.
5. Adopt the implemented external deployment/observation receipt v1 in owning
   program upload and capture workflows. The current contract binds candidates,
   USB identity, protocol bytes, raw text captures, and exact assertions while
   leaving calibration for a later separately scoped schema.
6. Design simulation bindings as an independent consumer of the same physical
   contracts and hardware identities.
7. Compare simulated predictions with runtime observations without allowing
   either evidence stream to rewrite authored source or static declarations.

Driver execution, telemetry collection, simulation accuracy, deployment,
flashing, electrical safety, and physical acceptance remain separately proven
boundaries.

## Long-Term Direction

- After core maturity, extend the language/model surface toward physics-oriented metadata and tighter IDE-integrated analysis workflows.

## Maturity Note

`fisiCs` is under active development and remains experimental.
`clang` is still used as a reference/baseline compiler in validation workflows.
