// SPDX-License-Identifier: Apache-2.0

# Extensions Scaffold

This subtree owns non-core overlay behavior for `fisiCs`.

Rules:

- Core C parsing and semantics remain authoritative.
- Overlay features are explicitly feature-gated.
- Overlay metadata is attached beside core C types, not merged into `ParsedType` or `TypeInfo`.
- The first concrete overlay lane is physics units via `[[fisics::dim(...)]]`.

Phase 2 scope:

- framework routing
- units metadata model
- declaration attribute registration
- semantic validation scaffold

Out of scope here:

- runtime-aware lowering
- unit-conversion semantics
- custom sigil syntax
