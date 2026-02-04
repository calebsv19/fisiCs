# Conformance Strategy

## Scope
Explicitly track what is supported, simplified, or intentionally omitted.

## Validate
- Feature matrix for C99 core coverage.
- Warning policy and implementation-defined choices.
- Undefined behavior policy (allow vs diagnose).

## Acceptance Checklist
- Feature matrix lists supported/partial/unsupported items.
- Warning policy and diagnostic philosophy are documented.
- Implementation-defined and undefined behaviors are noted.
- Each test bucket maps to one or more features.

## Deliverables
1) `tests/final/meta/feature_matrix.md`
   - High-level C99 feature coverage status.
2) `tests/final/meta/feature_map.json`
   - Mapping of tests to feature tags.

## Expected Outputs
- Documentation files only (no test runner changes).
