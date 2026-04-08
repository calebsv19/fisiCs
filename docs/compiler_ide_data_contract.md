# Compiler IDE Data Contract

This document defines the public contract between `fisiCs` compiler analysis output and IDE consumers.

Contract ID: `fisiCs.analysis.contract`
Current Contract Version: `1.2.0`
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

Interpretation:

- `partial=true` means consumers must avoid assuming complete semantic coverage.
- `fatal=true` means analysis is incomplete and UX should be non-authoritative.
- `partial=false` and `fatal=false` indicates full successful analysis for this input.

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
