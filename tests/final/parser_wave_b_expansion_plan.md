# Parser Wave B Expansion Plan (C99 -> C17 Test Surface)

This note tracks the next parser-focused expansion pass after parser-diag export
enablement.

## Goal
- Increase parser grammar/recovery coverage density in active final suite.
- Prefer parser diagnostic tuple assertions (`.pdiag`) for malformed syntax.
- Keep default CLI output unchanged; use diagnostics JSON export path via harness.

## Wave Order
1) `09-statements-controlflow`
2) `04-declarations`
3) `05-expressions`
4) `11-functions-calls`
5) `12-diagnostics-recovery` (cross-error ordering/suppression)

## Implementation Status (Current Pass)
- [x] `09` parser-diag expansion wave added (`wave11`)
- [x] `04` parser-diag expansion wave added (`wave7`)
- [x] `05` parser-diag expansion wave added (`wave6`)
- [x] `11` parser-diag expansion wave added (`wave3`)
- [x] `12` mixed parser+semantic ordering wave added (`wave6`)
- [x] New wave IDs validated green in harness

## Target Gaps

### 09 Statements / Control Flow
- Empty control expressions (`if ()`, `while ()`).
- Broken `for` header separators.
- Malformed `switch` grammar sync points.
- Label/statement boundary edge at EOF.

### 04 Declarations
- Struct member declaration syntax failures.
- Bitfield syntax failures.
- Enum separator/body malformed forms.
- Deep declarator parenthesis failures.
- Array parameter `static` malformed forms.

### 05 Expressions
- Postfix malformed chains (`.`, `->`, call arg list, subscript closure).
- Ternary malformed variants (missing colon/operand, GNU omitted-middle reject).
- Nested grouping closure failures.

### 11 Functions / Calls
- Parameter list grammar malformed forms (comma/ellipsis/paren failures).
- Prototype/declaration malformed forms with follow-up declaration recovery.
- Function-pointer parameter malformed declarators.

### 12 Diagnostics Recovery
- Mixed parser + semantic error files.
- Primary parser diagnostic ordering anchors.
- Recovery continuation and parser cascade bounds under mixed failures.

## Promotion Rule
- Add as new `*-parserdiag.json` wave manifests.
- Generate deterministic `.pdiag` expectations.
- Run targeted new IDs first, then run wider bucket sweep.
- Record wave additions in bucket docs and checklist run log.
