// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Extensions/extension_hooks.h"

struct Scope;
struct ASTNode;

/*
 * Expression-side units results live in the extension-state side table keyed
 * by AST node for the lifetime of one compile pipeline run.
 *
 * Current Phase 5 Slice 2 behavior is intentionally limited to inference:
 * - bare number/char literals are dimensionless
 * - direct declaration-owned literals may adopt an owning declaration dim
 * - identifier references may inherit resolved declaration-owned dimensions
 * - core arithmetic wrapper forms may infer a result dimension when it is
 *   locally provable without issuing mismatch diagnostics
 *
 * Legality diagnostics remain a later slice so this pass can grow inference
 * first without entangling it with the first extension error policy.
 *
 * Current Phase 5 Slice 3 behavior adds the first mismatch diagnostics:
 * - simple `=` assignments emit extension diagnostics on dimension mismatch
 * - `+` and `-` emit extension diagnostics on dimension mismatch
 *
 * Current Phase 5 Slice 4 behavior extends legality to comparisons:
 * - `==`, `!=`, `<`, `<=`, `>`, and `>=` emit extension diagnostics on
 *   dimension mismatch
 * - legal comparisons produce a dimensionless result
 *
 * Current Phase 5 Slice 5 behavior closes the first semantic hardening lane:
 * - matching-dimension ternary branches preserve that shared dimension
 * - mismatched ternary branches emit the reserved `4106` extension diagnostic
 * - `*` and `/` now emit the reserved `4105` diagnostic when exponent math
 *   exceeds the int8-backed `Dim8` range instead of failing silently
 */
void fisics_units_run_expr_semantics(struct ASTNode* root, struct Scope* globalScope);
