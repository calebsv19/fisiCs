// SPDX-License-Identifier: Apache-2.0

# Extensions Scaffold

This subtree owns non-core overlay behavior for `fisiCs`.

Rules:

- Core C parsing and semantics remain authoritative.
- Overlay features are explicitly feature-gated.
- Overlay metadata is attached beside core C types, not merged into `ParsedType` or `TypeInfo`.
- The first concrete overlay lane is physics units via `[[fisics::dim(...)]]` plus the newer declaration-side `[[fisics::unit(...)]]` attachment.
- Extension-owned `ASTNode*` keys and copied annotation strings are valid only for one compile pipeline run.
- Phase 3 owns the node-keyed expression side table and may only record expression results that are locally provable from the current scaffold state.

Phase 2 scope:

- framework routing
- units metadata model
- declaration attribute registration
- semantic validation scaffold

Current scaffold ownership:

- declaration-side units annotations are normalized into one canonical extension-state slot per owner declaration node
- repeated `[[fisics::dim(...)]]` or `[[fisics::unit(...)]]` use against the same owner node is tracked as duplicate scaffold input and rejected during semantic validation
- canonical text and parsed `Dim8` payloads are compiler-owned scaffold data, not a stable exported API
- concrete unit identity now also lives on the same owner annotation slot as an internal registry pointer plus copied source text
- expression-side units results are a separate node-keyed side table used for inferred expression dimensions and debug visibility
- expression-side identifier bindings are stored as extension-owned references to stable declaration annotations, not raw lexical scope objects or transient function-scope `Symbol*` pointers

Current debug visibility contract:

- `--dump-sema` shows declaration attachments, symbol attachments, and expression-side units results in a debug-only format
- symbol lines report attachment state, not full semantic acceptance:
  - `resolved` means the declaration annotation canonicalized into a valid `Dim8`
  - `pending` means the overlay metadata was attached but rejected or left unresolved by the current scaffold pass
  - `unit-resolved` / `unit-pending` report the independent validation state of `[[fisics::unit(...)]]`
- expression-side result dumps now reflect the first inference lane from Phase 5 Slice 2:
  - dimensionless number and character literals are always visible as resolved local expression results
  - declaration-owned literal initializers only inherit units when the owning annotation resolved successfully
  - identifier references inherit units only when core semantic analysis bound them to a resolved declaration-owned units annotation
  - simple wrapper/arithmetic forms such as casts, unary `+/-`, `=`, `+`, `-`, `*`, `/`, comparisons, and comma expressions may show inferred result dimensions when they are locally provable
- Phase 6 expands the readable dimension vocabulary without changing the underlying canonical meaning:
  - symbolic atoms such as `m`, `kg`, and `s` remain valid
  - aliases such as `length`, `distance`, `duration`, `speed`, `energy`, and `work` now resolve through the same canonical `Dim8` path
  - English concrete-unit words such as `meter`, `second`, `minute`, and `joule` are intentionally rejected here and reserved for the future `[[fisics::unit(...)]]` lane
- Phase 7 Slice 1 seeds that future lane as internal registry data only:
  - concrete units now have a family-grouped contract with canonical name, symbol, `Dim8`, canonical-scale metadata, offset slot, and alias table
  - the current `dim(...)` deferred-word rejection path is backed by that registry, so later `unit(...)` syntax work can attach to one source of truth
- Phase 7 Slice 2 makes `[[fisics::unit(...)]]` live as a validation-only declaration attachment:
  - `unit(...)` currently requires `dim(...)`
  - the unit must exist in the seeded registry and match the declaration `Dim8` exactly
  - unit identity does not yet participate in arithmetic or conversion behavior
- Phase 7 Slice 3 promotes resolved declaration/symbol unit identity into the public attachment lane:
  - frontend contract `1.6.0` adds optional `unit_name`, `unit_symbol`, `unit_family`, and `unit_resolved` fields on `units_attachments`
  - producers advertise `FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE` only when at least one exported attachment carries resolved concrete unit identity
  - expression semantics remain dimension-first; concrete unit identity is observable metadata, not an arithmetic policy input
- Phase 7 Slice 4 freezes that public concrete-unit lane before conversions:
  - frontend contract `1.7.0` adds optional `unit_source_text` so IDE/agent consumers can preserve the original source spelling beside the canonical unit name
  - canonical scale/offset and other conversion mechanics remain intentionally internal until Phase 8
- Phase 8 Slice 3 adds the first explicit conversion helper lane without widening the public contract:
  - `fisics_convert_unit(value, "target_unit")` and `__builtin_fisics_convert_unit(...)` now form the explicit-only concrete-unit conversion path
  - conversion requires a resolved source unit, a valid target registry unit, matching canonical `Dim8`, and a conversion-supported family
  - the first helper lane is intentionally floating-scalar only
  - implicit same-dimension concrete-unit mixing is now rejected in declaration initializers, assignment, `+`, `-`, comparisons, and ternary result merge
  - the closeout coverage now distinguishes invalid target text from known-but-incompatible target units

Current extension diagnostic namespace:

- `4000-4099` is the generic extension/bootstrap namespace
- `4001-4007` are already-live scaffold/bootstrap units diagnostics:
  - overlay disabled
  - invalid `fisics::dim(...)`
  - duplicate declaration annotation
  - invalid `fisics::unit(...)`
  - duplicate `fisics::unit(...)`
  - bare `unit(...)` without `dim(...)`
  - `unit(...)` dimension mismatch against `dim(...)`
- `4100-4199` is the Phase 5 physics-units semantic namespace
- live units semantic diagnostics now include:
  - `4101` addition dimension mismatch
  - `4102` subtraction dimension mismatch
  - `4103` assignment dimension mismatch
  - `4104` comparison dimension mismatch
  - `4105` exponent-overflow in units multiplication/division algebra
  - `4106` unsupported units operation, currently used for ternary branch mismatches
  - `4107` implicit concrete-unit conversion requires an explicit helper
  - `4108` invalid explicit conversion target unit
  - `4109` incompatible explicit conversion path
  - `4110` explicit conversion requires a resolved source unit
  - `4111` explicit conversion currently requires a floating scalar source
- extension diagnostics stay in `FISICS_DIAG_CATEGORY_EXTENSION`; they do not reuse core parser/semantic code ranges

Out of scope here:

- runtime-aware lowering
- custom sigil syntax
