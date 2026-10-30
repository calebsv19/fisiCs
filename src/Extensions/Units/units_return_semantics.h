// SPDX-License-Identifier: Apache-2.0
#pragma once
struct ASTNode;
struct CompilerContext;
void fisics_units_validate_return(struct CompilerContext* ctx,
    struct ASTNode* function, struct ASTNode* statement);
