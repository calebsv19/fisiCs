# Translation-Unit Metadata Export Fixtures

`relationships.c` is the frozen translation-unit projection input. It covers authored and
fallback source identities, singular properties, compatible-redeclaration edge
merge, distinct edge union, resolved targets, and external targets.

`relationships.expected.json.in` freezes schema-v1 structure, key order, array
order, null policy, and enum spelling. Tokens beginning and ending with `@` are
limited to environment-derived values: normalized absolute path, input hash and
length, producer version, frontend stable IDs, and parser-owned source columns.
The metadata JSON integration test must substitute those values and compare exact
bytes; it must not structurally rewrite or reorder the template.

Relationship occurrence ranges use the accepted declaration-owner range
retained by the declaration relationship model. The initial parser does not retain per-attribute token ranges,
so the contract does not synthesize line positions from attribute ordering.

The legacy ABI baseline lives in
`tests/unit/frontend_api_contract_semantic_metadata_sm5a_baseline.c`. Its mirror
structure is intentionally frozen at the legacy `FisicsAnalysisResult` layout.
