// SPDX-License-Identifier: Apache-2.0

# Extensions Scaffold

This subtree owns non-core overlay behavior for `fisiCs`.

Rules:

- Core C parsing and semantics remain authoritative.
- Overlay features are explicitly feature-gated.
- Overlay metadata is attached beside core C types, not merged into `ParsedType` or `TypeInfo`.
- The first concrete overlay lane is physics units via `[[fisics::dim(...)]]` plus the newer declaration-side `[[fisics::unit(...)]]` attachment.
- The semantic-metadata lane owns opt-in `[[fisics::id(...)]]`, `[[fisics::domain(...)]]`, and `[[fisics::role(...)]]` declaration records plus repeatable `[[fisics::produces(...)]]` and `[[fisics::consumes(...)]]` relationship edges.
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
- semantic metadata records are sorted by first symbol source location and name,
  independently of symbol-table bucket order
- semantic relationship edges merge across compatible redeclarations, resolve
  only against valid authored IDs in the active translation unit, preserve
  other canonical targets as `external`, and use a stable source projection
  when no authored ID exists
- symbol lines report attachment state, not full semantic acceptance:
  - `resolved` means the declaration annotation canonicalized into a valid `Dim8`
  - `pending` means the overlay metadata was attached but rejected or left unresolved by the current scaffold pass
  - `unit-resolved` / `unit-pending` report the independent validation state of `[[fisics::unit(...)]]`
- expression-side result dumps reflect the implemented local inference lane:
  - dimensionless number and character literals are always visible as resolved local expression results
  - declaration-owned literal initializers only inherit units when the owning annotation resolved successfully
  - identifier references inherit units only when core semantic analysis bound them to a resolved declaration-owned units annotation
  - simple wrapper/arithmetic forms such as casts, unary `+/-`, `=`, `+`, `-`, `*`, `/`, comparisons, and comma expressions may show inferred result dimensions when they are locally provable
- the readable dimension vocabulary expands without changing the underlying canonical meaning:
  - symbolic atoms such as `m`, `kg`, and `s` remain valid
  - aliases such as `length`, `distance`, `duration`, `speed`, `energy`, and `work` now resolve through the same canonical `Dim8` path
  - English concrete-unit words such as `meter`, `second`, `minute`, and `joule` are intentionally rejected here and reserved for the future `[[fisics::unit(...)]]` lane
- the concrete-unit registry establishes the internal data foundation:
  - concrete units now have a family-grouped contract with canonical name, symbol, `Dim8`, canonical-scale metadata, offset slot, and alias table
  - the current `dim(...)` deferred-word rejection path is backed by that registry, so later `unit(...)` syntax work can attach to one source of truth
- the registry integrity contract locks these rules without widening behavior:
  - canonical exported names are singular lowercase `snake_case`
  - symbols are canonical short spellings and are expected to stay globally unique
  - aliases are source-acceptance spellings only and must not collide with any canonical name, symbol, or other alias
  - every unit in a family must carry that family's exact canonical `Dim8`
  - canonical-family units use `scale_to_canonical = 1.0`
  - the offset slot is reserved for a future dedicated lane and should remain `0.0` in the current registry
- a maintainer-facing audit seam enforces that policy:
  - `fisics_unit_registry_validate(...)` audits the built-in registry and reports aggregate family/unit/alias counts plus total issue count
  - `fisics_unit_registry_validate_table(...)` exists so narrow tests can inject bad tables and prove duplicate-name, duplicate-symbol, alias-collision, family/dimension mismatch, scale, and offset failures mechanically
- Structural registry coverage now includes the first geometry-derived families needed by real solvers:
  - `area` (`L^2`) with canonical units such as `square_meter`
  - `second_moment_of_area` (`L^4`) with canonical units such as `meter_to_fourth`
  - dimension aliases such as `cross_sectional_area` and `area_moment_of_inertia` resolve directly through `dim(...)`
  - concrete unit words such as `square_meter` and `meter_to_fourth` remain deferred unit tokens rather than direct dimension aliases
- `[[fisics::unit(...)]]` is a validation-only declaration attachment:
  - `unit(...)` currently requires `dim(...)`
  - the unit must exist in the seeded registry and match the declaration `Dim8` exactly
  - unit identity does not yet participate in arithmetic or conversion behavior
- The concrete-unit frontend export promotes resolved declaration/symbol unit identity into the public attachment lane:
  - frontend contract `1.6.0` adds optional `unit_name`, `unit_symbol`, `unit_family`, and `unit_resolved` fields on `units_attachments`
  - producers advertise `FISICS_CONTRACT_CAP_EXTENSION_UNITS_CONCRETE` only when at least one exported attachment carries resolved concrete unit identity
  - expression semantics remain dimension-first; concrete unit identity is observable metadata, not an arithmetic policy input
- The source-spelling addition completes that public concrete-unit lane before conversions:
  - frontend contract `1.7.0` adds optional `unit_source_text` so IDE/agent consumers can preserve the original source spelling beside the canonical unit name
  - canonical scale/offset and other conversion mechanics remain intentionally internal to the explicit-conversion contract
- IDE bridge local-units export widens the declaration attachment metadata:
  - frontend contract `1.8.0` adds `has_symbol_stable_id` and source file/range fields so local declarations can navigate without exported stable symbol ids
  - the attachment lane still contains only compiler-known declaration-side annotations; expression-result browsing and suffix heuristics remain out of contract
- Semantic metadata uses a separate public export container without
  resizing the legacy result:
  - frontend contract `1.9.0` adds `FisicsAnalysisResultEx` and capability bit
    `FISICS_CONTRACT_CAP_EXTENSION_SEMANTIC_METADATA`
  - CLI export uses `--emit-semantic-metadata-json <path>` with explicit
    `--overlay=semantic-metadata`, exactly one C input, deterministic ordering,
    and atomic destination replacement
  - project aggregation, strict unresolved references, code generation, and
    runtime behavior remain outside this lane
- the explicit conversion helper lane adds conversion without implicit mixing:
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
- `4200-4299` is the semantic-metadata namespace; `4201-4206` and `4209` are
  live for disabled, invalid, target, singular-duplicate,
  redeclaration-conflict, duplicate-ID, and duplicate-relationship cases
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
