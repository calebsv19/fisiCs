// SPDX-License-Identifier: Apache-2.0
#pragma once
struct ASTNode;
struct CompilerContext;
struct ParsedType;

void fisics_units_capture_access_type(struct CompilerContext* ctx,
    struct ASTNode* node, const struct ParsedType* type);
void fisics_units_record_access_result(struct CompilerContext* ctx,
    struct ASTNode* node);
