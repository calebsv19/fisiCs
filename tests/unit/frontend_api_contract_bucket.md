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
| `frontend_api_contract_metadata` | Validate required contract metadata fields and semver floor (`major=1`, `minor>=4`) | pass |
| `frontend_api_contract_hash_stability` | Validate `source_hash` determinism for identical source and change on different source | pass |
| `frontend_api_contract_parent_stable_id` | Validate owned symbols carry `parent_stable_id` in v1.2+ lane | pass |
| `frontend_api_contract_diagnostics_taxonomy` | Validate additive diagnostics taxonomy fields (`severity_id`, `category_id`, `code_id`) in v1.3+ lane | pass |
| `frontend_api_contract_capabilities` | Validate additive contract capability bitmask lane in v1.4+ (`contract.capabilities`) | pass |

## Policy

- Failing checks are treated as contract conformance regressions.
- Regressions should be added to the active private failing ledger before fix work.
- Keep this bucket additive and backward-compatible with v1 contract policy.
