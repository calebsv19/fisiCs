# Extension Portability Parity Fixtures

These two translation units express the same units and semantic-metadata
meaning through different source spellings:

- `native_equivalent.c` uses canonical direct `[[fisics::...]]` attributes;
- `macro_equivalent.c` uses `<fisics/extensions.h>` and all seven public
  `FISICS_*` macros.

`tests/integration/run_extension_portability_macros.sh` emits translation-unit semantic and
units metadata for both files and compares normalized semantic projections.
Source paths, hashes, ranges, binding IDs, and symbol-stable IDs are
intentionally excluded because they identify different source files. Authored
IDs, symbol meaning, roles, domains, relationships, dimensions, and concrete
units must match.

The native fixture is retained permanently as parser/attribute evidence. It is
not a shared-source migration candidate.
