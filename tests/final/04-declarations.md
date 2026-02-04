# Parser: Declarations

## Scope
Declarators, type qualifiers, aggregates, and C99 declaration features.

## Validate
- Pointers/arrays/functions with nested declarators.
- struct/union/enum definitions and forward decls.
- Designated initializers and compound literals.
- Bit-fields, flexible arrays, VLAs, inline (if supported).

## Test Ideas
## Acceptance Checklist
- Nested declarators print a stable canonical type string.
- Storage class and qualifiers are preserved and ordered.
- struct/union/enum declarations handle forward refs and definitions.
- Designated initializers parse with correct target fields.
- Bit-fields and flexible arrays parse without crashing.

## Test Cases (initial list)
1) `04__nested_declarators`
   - Pointers, arrays, functions combined.
2) `04__storage_and_qualifiers`
   - static/extern/typedef/const/volatile/restrict combos.
3) `04__struct_union_enum`
   - forward declaration + definition + enum values.
4) `04__designated_initializers`
   - .field and [index] designators.
5) `04__bitfields_flex`
   - bit-fields and flexible array member.

## Expected Outputs
- AST/Diagnostics goldens for parsing and declaration structure.
