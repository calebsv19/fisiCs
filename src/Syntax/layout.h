// SPDX-License-Identifier: Apache-2.0

#ifndef LAYOUT_H
#define LAYOUT_H

#include <stddef.h>
#include <stdbool.h>
#include "Compiler/compiler_context.h"
#include "Parser/Helpers/parsed_type.h"
#include "scope.h"
#include "AST/ast_node.h"

bool layout_struct_union(CompilerContext* ctx,
                         Scope* scope,
                         CCTagKind kind,
                         const char* name,
                         size_t* sizeOut,
                         size_t* alignOut);

bool size_align_of_parsed_type(ParsedType* type,
                               Scope* scope,
                               size_t* sizeOut,
                               size_t* alignOut);

#endif // LAYOUT_H
