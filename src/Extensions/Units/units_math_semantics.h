// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <stdbool.h>
struct ASTNode;
struct CompilerContext;
struct Symbol;

typedef enum { FISICS_UNITS_MATH_NONE, FISICS_UNITS_MATH_SQRT,
    FISICS_UNITS_MATH_FABS, FISICS_UNITS_MATH_HYPOT, FISICS_UNITS_MATH_FMAX,
    FISICS_UNITS_MATH_EXPM1, FISICS_UNITS_MATH_LOG, FISICS_UNITS_MATH_FMIN, FISICS_UNITS_MATH_ATAN2
} FisicsUnitsMathOperation;
typedef struct { struct ASTNode* node; FisicsUnitsMathOperation operation; }
    FisicsUnitsMathCall;

/* Capture the resolved direct callee; do not reconstruct it by global name. */
void fisics_units_note_math_call(struct CompilerContext* ctx, struct ASTNode* call,
    const struct Symbol* callee, bool realArguments);
/* True means this is a recognized math call, even if input metadata is unknown or
 * rejected. Such calls must not fall through to unrelated result inference. */
bool fisics_units_record_math_result(struct CompilerContext* ctx, struct ASTNode* call);

/* Read the captured identity; no name lookup or new trust decisions. */
FisicsUnitsMathOperation fisics_units_math_operation(const struct CompilerContext *,const struct ASTNode *);
