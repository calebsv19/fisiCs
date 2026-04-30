# Frontend Contract Bucket

Bucket ID: `16-contract-interface`  
Scope: compiler-to-IDE contract conformance for `fisiCs.analysis.contract` v1.x

## Purpose

Provide a focused, low-cost conformance lane for the frontend contract surface so
contract regressions are caught before broader IDE/runtime integration checks.

## Run

```bash
make frontend-contract-test
```

## Matrix

| Test Binary | Intent | Expected Outcome |
|---|---|---|
| `frontend_api_contract_metadata` | Validate required contract metadata fields and semver floor (`major=1`, `minor>=7`) | pass |
| `frontend_api_contract_hash_stability` | Validate `source_hash` determinism for identical source and change on different source | pass |
| `frontend_api_contract_parent_stable_id` | Validate owned symbols carry `parent_stable_id` in v1.2+ lane | pass |
| `frontend_api_contract_diagnostics_taxonomy` | Validate additive diagnostics taxonomy fields (`severity_id`, `category_id`, `code_id`) in v1.3+ lane | pass |
| `frontend_api_contract_extension_diag_namespace` | Validate extension-owned diagnostics stay in the extension taxonomy lane and keep the stable bootstrap code ids | pass |
| `frontend_api_contract_capabilities` | Validate additive contract capability bitmask lane in v1.4+ (`contract.capabilities`) | pass |
| `frontend_api_contract_units_attachment_lane` | Validate the v1.7 units attachment lane stays null/empty and unadvertised when no valid overlay export is present | pass |
| `frontend_api_contract_units_attachment_overlay_empty` | Validate overlay enablement alone does not advertise or populate the units lane when no units annotations are present | pass |
| `frontend_api_contract_units_attachment_invalid_gated` | Validate malformed `fisics::dim(...)` input stays diagnostics-only and does not leak capability/payload export | pass |
| `frontend_api_contract_units_attachment_export` | Validate resolved `[[fisics::dim(...)]]` attachments export through the public lane and advertise the capability bit | pass |
| `frontend_api_contract_units_attachment_concrete_export` | Validate resolved `[[fisics::unit(...)]]` metadata exports canonical unit identity plus preserved source spelling through the public units lane and advertises the concrete-unit capability bit | pass |
| `frontend_api_contract_units_registry_model` | Validate the Phase 7 unit-registry contract exposes family-grouped concrete units while `dim(...)` still treats them as deferred words | pass |

## Policy

- Failing checks are treated as contract conformance regressions.
- Regressions should be added to the active private failing ledger before fix work.
- Keep this bucket additive and backward-compatible with v1 contract policy.
