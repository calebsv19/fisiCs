// SPDX-License-Identifier: Apache-2.0

# Extensions Scaffold

This subtree owns non-core overlay behavior for `fisiCs`.

Rules:

- Core C parsing and semantics remain authoritative.
- Overlay features are explicitly feature-gated.
- Overlay metadata is attached beside core C types, not merged into `ParsedType` or `TypeInfo`.
- The first concrete overlay lane is physics units via `[[fisics::dim(...)]]`.
- Extension-owned `ASTNode*` keys and copied annotation strings are valid only for one compile pipeline run.
- Phase 3 owns the node-keyed expression side table and may only record expression results that are locally provable from the current scaffold state.

Phase 2 scope:

- framework routing
- units metadata model
- declaration attribute registration
- semantic validation scaffold

Current scaffold ownership:

- declaration-side units annotations are normalized into one canonical extension-state slot per owner declaration node
- repeated `[[fisics::dim(...)]]` use against the same owner node is tracked as duplicate scaffold input and rejected during semantic validation
- canonical text and parsed `Dim8` payloads are compiler-owned scaffold data, not a stable exported API
- expression-side units results are a separate node-keyed side table used for inferred expression dimensions and debug visibility
- expression-side identifier bindings are stored as extension-owned references to stable declaration annotations, not raw lexical scope objects or transient function-scope `Symbol*` pointers

Current debug visibility contract:

- `--dump-sema` shows declaration attachments, symbol attachments, and expression-side units results in a debug-only format
- symbol lines report attachment state, not full semantic acceptance:
  - `resolved` means the declaration annotation canonicalized into a valid `Dim8`
  - `pending` means the overlay metadata was attached but rejected or left unresolved by the current scaffold pass
- expression-side result dumps now reflect the first inference lane from Phase 5 Slice 2:
  - dimensionless number and character literals are always visible as resolved local expression results
  - declaration-owned literal initializers only inherit units when the owning annotation resolved successfully
  - identifier references inherit units only when core semantic analysis bound them to a resolved declaration-owned units annotation
  - simple wrapper/arithmetic forms such as casts, unary `+/-`, `=`, `+`, `-`, `*`, `/`, comparisons, and comma expressions may show inferred result dimensions when they are locally provable

Current extension diagnostic namespace:

- `4000-4099` is the generic extension/bootstrap namespace
- `4001-4003` are already-live scaffold units diagnostics:
  - overlay disabled
  - invalid `fisics::dim(...)`
  - duplicate declaration annotation
- `4100-4199` is the Phase 5 physics-units semantic namespace
- live units semantic diagnostics now include:
  - `4101` addition dimension mismatch
  - `4102` subtraction dimension mismatch
  - `4103` assignment dimension mismatch
  - `4104` comparison dimension mismatch
  - `4105` exponent-overflow in units multiplication/division algebra
  - `4106` unsupported units operation, currently used for ternary branch mismatches
- extension diagnostics stay in `FISICS_DIAG_CATEGORY_EXTENSION`; they do not reuse core parser/semantic code ranges

Out of scope here:

- runtime-aware lowering
- unit-conversion semantics
- custom sigil syntax
