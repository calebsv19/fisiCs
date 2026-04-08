# Compiler IDE Data Contract

This document defines the public contract between `fisiCs` compiler analysis output and IDE consumers.

Contract ID: `fisiCs.analysis.contract`
Current Contract Version: `1.4.0`
Status: active

## Goals

- Provide a stable, explicit machine-readable boundary between compiler and IDE.
- Prevent silent IDE regressions when compiler internals evolve.
- Allow forward growth via compatibility rules and stability lanes.

## Contract Surface

Primary producer API:

- `fisics/src/fisics_frontend.h`
- `fisics/src/fisics_frontend.c`

Primary consumer lanes:

- `ide/src/core/Analysis/project_scan.c`
- `ide/src/core/Analysis/fisics_bridge.c`

### Payload Groups

`FisicsAnalysisResult` contains:

- Contract metadata (`contract`)
- Diagnostics (`diagnostics`)
- Symbols (`symbols`)
- Includes (`includes`)
- Tokens (`tokens`, optional lane)

## Stability Lanes

### Stable (v1 Core)

- Contract metadata fields and interpretation rules
- Diagnostics core shape and severity semantics
- Include entries (including unresolved includes)
- Symbol core shape (name, kind, location)

### Experimental

- Macro symbol behavior/details
- Deep semantic metadata not yet listed as stable
- Any field explicitly marked experimental in API comments/docs

## Compatibility Policy

Semver applies to contract version:

- `MAJOR`: breaking change (consumer behavior may require updates)
- `MINOR`: backward-compatible additions (new optional fields/lane extensions)
- `PATCH`: clarifications/fixes without shape or behavior breakage

IDE compatibility behavior:

- Same major: normal mode
- Newer producer major than consumer supports: degraded mode + warning
- Older producer major: best-effort consume + warning if needed

## Result Metadata Contract

Contract metadata must include:

- `contract_id`
- `contract_semver` (`major`, `minor`, `patch`)
- `producer_name`
- `producer_version`
- `mode` (`strict` or `lenient`)
- `partial` (true when output is best-effort/incomplete)
- `fatal` (true when analysis encountered fatal failure)
- trust metadata:
  - `source_hash` (content hash of analyzed buffer)
  - `source_length`
- `capabilities` (bitmask describing optional/additive lanes advertised by the producer)

Interpretation:

- `partial=true` means consumers must avoid assuming complete semantic coverage.
- `fatal=true` means analysis is incomplete and UX should be non-authoritative.
- `partial=false` and `fatal=false` indicates full successful analysis for this input.

### Contract Capabilities (v1.4)

This lane adds explicit producer-advertised capability flags so consumers can gate optional behavior without relying only on minor-version inference.

Current capability bits:

- `FISICS_CONTRACT_CAP_DIAGNOSTICS`
- `FISICS_CONTRACT_CAP_INCLUDES`
- `FISICS_CONTRACT_CAP_SYMBOLS`
- `FISICS_CONTRACT_CAP_TOKENS`
- `FISICS_CONTRACT_CAP_SYMBOL_PARENT_STABLE_ID`
- `FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY`

Capability table:

| Flag | Meaning | Typical Consumer Behavior |
|---|---|---|
| `FISICS_CONTRACT_CAP_DIAGNOSTICS` | Diagnostic lane present | Ingest diagnostics as authoritative analysis feedback. |
| `FISICS_CONTRACT_CAP_INCLUDES` | Include graph lane present | Update include graph/index from emitted include edges. |
| `FISICS_CONTRACT_CAP_SYMBOLS` | Symbol lane present | Enable symbol index/tree payloads; disable symbol lane if absent. |
| `FISICS_CONTRACT_CAP_TOKENS` | Token lane present | Enable token consumers (lexical overlays/highlighting support). |
| `FISICS_CONTRACT_CAP_SYMBOL_PARENT_STABLE_ID` | Parent symbol identity link lane present | Prefer `parent_stable_id` ownership linkage over name-only fallback. |
| `FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY` | Normalized diagnostics taxonomy lane present | Prefer `severity_id`/`category_id`/`code_id` when available. |

Consumer behavior:

- For contract `1.4+`, capability flags are authoritative for optional lanes.
- For pre-`1.4` payloads, consumers may infer capability support by minor-version policy.
- Missing optional capabilities must not force full degraded mode under major `1`; only unsupported contract major/id should trigger degraded ingestion.

Compatibility matrix (major `1` consumers):

| Producer contract | Capability source | Symbols/Tokens behavior | Diagnostics taxonomy behavior |
|---|---|---|---|
| `1.2.x` | inferred by minor policy | enabled by inferred support | taxonomy optional/legacy-safe |
| `1.3.x` | inferred by minor policy | enabled by inferred support | `severity_id`/`category_id`/`code_id` expected additive lane |
| `1.4.x+` | explicit `contract.capabilities` | gated by advertised `FISICS_CONTRACT_CAP_SYMBOLS` / `FISICS_CONTRACT_CAP_TOKENS` | gated by `FISICS_CONTRACT_CAP_DIAGNOSTIC_TAXONOMY` |

## Path Rules

- Contract file paths should be absolute when emitted to IDE consumers.
- Includes must preserve unresolved entries; unresolved entries may have null `resolved_path`.

## Diagnostics Contract

Stable expectations:

- severity (`error`, `warning`, `note` via enum mapping)
- location (`file`, `line`, `column`, `length`)
- `code` (stable enum lane for externally consumable diagnosis classes)
- `message`
- optional `hint`
- JSON consumers may expose normalized taxonomy lanes in addition to legacy severity text:
  - `severity_name` (`info`/`warning`/`error`)
  - `severity_id` (stable numeric severity enum in consumer-facing transport)
  - `category` (for example `build` vs `analysis` in IDE aggregation)

### Diagnostic Taxonomy Identity (v1.3)

This lane extends diagnostics with additive stable IDs without breaking existing consumers.

Stable additive fields:

- `severity_id`:
  - stable numeric severity identity (`info=0`, `warning=1`, `error=2`).
- `category_id`:
  - stable numeric diagnostic category identity (`analysis`, `parser`, `semantic`, `preprocessor`, etc.).
- `code_id`:
  - stable external diagnostic code id lane for tooling that prefers explicit numeric identity fields.

Consumer behavior:

- Existing diagnostics fields (`kind`/legacy severity text, `code`, `message`, etc.) remain valid.
- Consumers may prefer `severity_id`/`category_id`/`code_id` when present.
- Missing additive taxonomy fields must not be treated as contract failure under major `1`.

### Diagnostic Code Stability

`fisiCs` uses a stable diagnostic code namespace for v1-facing consumers.
Internal diagnostic generation may evolve, but published stable code values must remain compatible within major version.

## Symbol Contract (v1)

Stable:

- `name`
- `kind`
- `file_path`
- `start/end` location
- ownership (`parent_name`, `parent_kind`) when present
- `stable_id` (deterministic symbol identity for cache/index matching; hex string when emitted through JSON IPC lanes)

## Symbol Graph Identity Contract (v1.2)

This lane extends symbol ownership linking without breaking existing consumers.

Stable expectations:

- Existing ownership fields remain valid:
  - `parent_name`
  - `parent_kind`
- Existing symbol identity remains stable:
  - `stable_id`

New additive link lane (optional but recommended):

- `parent_stable_id`:
  - deterministic stable ID of the direct parent/owner symbol when available.
  - `0`/missing means no stable parent link is currently available.

Consumer behavior:

- Consumers must continue to support name/kind ownership matching when `parent_stable_id` is missing.
- When `parent_stable_id` is present, consumers should prefer it over name-only matching for graph linkage/caching.
- Missing additive link fields must not be treated as contract failure under major `1`.

## Includes Contract (v1)

- Includes are mandatory output lane.
- Both resolved and unresolved includes must be emitted.
- `origin` and `resolved` state are authoritative for include graph/index consumers.

## Tokens Contract

- Tokens are optional for consumers.
- Missing tokens must not be treated as failure.
- Intended for editor lexing/highlighting support lanes.
- Token kind semantics are stable through enum identity in `FisicsTokenKind`.
- JSON-facing consumers should prefer dual representation when available:
  - `kind_id` (numeric enum value)
  - `kind` (stable lowercase string label)

## Change Governance

- Any contract-shape or compatibility change requires maintainer review.
- Public docs and API comments must be updated in the same change set.
- Contract version must be advanced according to semver policy.
