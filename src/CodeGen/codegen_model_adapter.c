// SPDX-License-Identifier: Apache-2.0

#include "codegen_model_adapter.h"

#include "Syntax/semantic_model.h"
#include "Syntax/semantic_model_details.h"
#include "scope.h"
#include "Parser/Helpers/designated_init.h"

#include <stdlib.h>
#include <string.h>

static size_t countSymbolsOfKind(const SemanticModel* model, SymbolKind kind) {
    size_t count = 0;
    const Scope* scope = semanticModelGetGlobalScope(model);
    if (!scope) return 0;
    for (int i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
        for (Symbol* sym = scope->table.buckets[i]; sym; sym = sym->next) {
            if (sym->kind == kind) {
                ++count;
            }
        }
    }
    return count;
}

size_t codegenModelGetStructCount(const SemanticModel* model) {
    if (!model) return 0;
    return semanticModelGetStructCount(model);
}

static void fillStructFields(const ASTNode* node, CGStructField* buffer, size_t bufferSize, size_t* outCount) {
    size_t count = 0;
    if (!node || !buffer) {
        *outCount = 0;
        return;
    }
    ASTNode* const* fields = node->structDef.fields;
    size_t fieldDeclCount = node->structDef.fieldCount;
    for (size_t i = 0; i < fieldDeclCount && count < bufferSize; ++i) {
        const ASTNode* fieldDecl = fields[i];
        if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
        for (size_t v = 0; v < fieldDecl->varDecl.varCount && count < bufferSize; ++v) {
            ASTNode* nameNode = fieldDecl->varDecl.varNames[v];
            buffer[count].name = (nameNode && nameNode->type == AST_IDENTIFIER)
                ? nameNode->valueNode.value
                : NULL;
            const ParsedType* parsed = astVarDeclTypeAt(fieldDecl, v);
            buffer[count].type = parsed ? *parsed : fieldDecl->varDecl.declaredType;
            ++count;
        }
    }
    *outCount = count;
}

void codegenModelFillStructInfo(const SemanticModel* model, CGStructInfo* buffer, size_t bufferSize) {
    if (!model || !buffer) return;
    size_t total = semanticModelGetStructCount(model);
    size_t index = 0;
    for (size_t i = 0; i < total && index < bufferSize; ++i) {
        const Symbol* sym = semanticModelGetStructByIndex(model, i);
        if (!sym) continue;
        const ASTNode* def = sym->definition;
        if (!def || (def->type != AST_STRUCT_DEFINITION && def->type != AST_UNION_DEFINITION)) continue;

        buffer[index].name = sym->name;
        buffer[index].symbol = sym;
        buffer[index].definition = def;
        buffer[index].isUnion = (def->type == AST_UNION_DEFINITION);

        size_t maxFields = 0;
        for (size_t f = 0; f < def->structDef.fieldCount; ++f) {
            const ASTNode* fieldDecl = def->structDef.fields[f];
            if (!fieldDecl || fieldDecl->type != AST_VARIABLE_DECLARATION) continue;
            maxFields += fieldDecl->varDecl.varCount;
        }

        CGStructField* fields = NULL;
        if (maxFields > 0) {
            fields = (CGStructField*)calloc(maxFields, sizeof(CGStructField));
            if (!fields) continue;
            size_t actual = 0;
            fillStructFields(def, fields, maxFields, &actual);
            buffer[index].fields = fields;
            buffer[index].fieldCount = actual;
        } else {
            buffer[index].fields = NULL;
            buffer[index].fieldCount = 0;
        }
        ++index;
    }
}

size_t codegenModelGetTypedefCount(const SemanticModel* model) {
    return countSymbolsOfKind(model, SYMBOL_TYPEDEF);
}

void codegenModelFillTypedefInfo(const SemanticModel* model, CGTypedefInfo* buffer, size_t bufferSize) {
    if (!model || !buffer) return;
    const Scope* scope = semanticModelGetGlobalScope(model);
    if (!scope) return;
    size_t count = 0;
    for (int i = 0; i < SYMBOL_TABLE_SIZE && count < bufferSize; ++i) {
        for (Symbol* sym = scope->table.buckets[i]; sym; sym = sym->next) {
            if (sym->kind == SYMBOL_TYPEDEF) {
                buffer[count].name = sym->name;
                buffer[count].type = sym->type;
                ++count;
                if (count == bufferSize) return;
            }
        }
    }
}
