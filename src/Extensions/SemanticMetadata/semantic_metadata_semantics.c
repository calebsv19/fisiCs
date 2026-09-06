// SPDX-License-Identifier: Apache-2.0

#include "Extensions/SemanticMetadata/semantic_metadata_semantics.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Compiler/diagnostics.h"
#include "Extensions/extension_hooks.h"
#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"
#include "Syntax/scope.h"
#include "Syntax/symbol_table.h"

typedef struct {
    FisicsSemanticMetadataEdgeKind kind;
    char* targetId;
} OwnerRelationship;

typedef struct {
    char* singularValues[3];
    size_t singularCounts[3];
    OwnerRelationship* relationships;
    size_t relationshipCount;
    size_t relationshipCapacity;
} OwnerMetadata;

static SourceRange node_loc(const ASTNode* node) {
    if (node && node->location.start.line > 0) return node->location;
    SourceRange loc = {0};
    loc.start.line = node ? node->line : 0;
    loc.end = loc.start;
    return loc;
}

static const char* kind_name(FisicsSemanticMetadataAttrKind kind) {
    const FisicsSemanticMetadataDescriptor* descriptor =
        fisics_semantic_metadata_descriptor(kind);
    return descriptor ? descriptor->displayName : "unknown";
}

static size_t kind_index(FisicsSemanticMetadataAttrKind kind) {
    return (size_t)kind - 1u;
}

static FisicsSemanticMetadataEdgeKind relationship_edge_kind(
    FisicsSemanticMetadataAttrKind kind) {
    return kind == FISICS_METADATA_ATTR_CONSUMES
               ? FISICS_METADATA_EDGE_CONSUMES
               : FISICS_METADATA_EDGE_PRODUCES;
}

static void diag_disabled(CompilerContext* ctx, const ASTNode* node, const char* name) {
    compiler_report_diag(ctx, node_loc(node), DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_DISABLED,
                         "enable with --overlay=semantic-metadata or FISICS_OVERLAY=semantic-metadata",
                         "fisics semantic metadata annotation '%s' requires the semantic-metadata overlay",
                         name);
}

static void diag_malformed(CompilerContext* ctx, const ASTNode* node,
                           const char* name, const char* detail) {
    compiler_report_diag(ctx, node_loc(node), DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_INVALID,
                         detail,
                         "invalid fisics::%s semantic metadata annotation",
                         name);
}

static void diag_invalid_target(CompilerContext* ctx, const ASTNode* node,
                                const char* name) {
    compiler_report_diag(ctx, node_loc(node), DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_INVALID_TARGET,
                         "the first semantic-metadata contract accepts functions and single-declarator file-scope variables",
                         "fisics::%s is not valid on this target",
                         name);
}

static void diag_duplicate_owner(CompilerContext* ctx, const ASTNode* node,
                                 const char* name) {
    compiler_report_diag(ctx, node_loc(node), DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_DUPLICATE,
                         "use at most one annotation of each semantic metadata kind per declaration owner",
                         "duplicate fisics::%s annotation on one declaration",
                         name);
}

static void diag_duplicate_relationship(CompilerContext* ctx,
                                        const ASTNode* node,
                                        const char* name,
                                        const char* targetId) {
    compiler_report_diag(ctx, node_loc(node), DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_DUPLICATE_RELATIONSHIP,
                         "keep one occurrence of each relationship kind and target on a declaration",
                         "duplicate fisics::%s relationship to '%s' on one declaration",
                         name, targetId ? targetId : "");
}

static void diag_redecl_conflict(CompilerContext* ctx, const ASTNode* node,
                                 const char* symbolName, const char* field) {
    compiler_report_diag(ctx, node_loc(node), DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_REDECL_CONFLICT,
                         "compatible redeclarations must repeat the same semantic metadata value",
                         "conflicting fisics::%s metadata across redeclarations of '%s'",
                         field, symbolName ? symbolName : "<unnamed>");
}

static void diag_duplicate_id(CompilerContext* ctx,
                              const FisicsSemanticMetadataRecord* record) {
    compiler_report_diag(ctx, record->location, DIAG_ERROR,
                         CDIAG_EXTENSION_METADATA_DUPLICATE_ID,
                         "authored semantic IDs must be unique among distinct symbols in one translation unit",
                         "duplicate authored semantic metadata id '%s'",
                         record->authoredId ? record->authoredId : "");
}

static bool record_reserve(FisicsExtensionState* state) {
    if (state->semanticMetadataRecordCount < state->semanticMetadataRecordCapacity) return true;
    size_t newCap = state->semanticMetadataRecordCapacity
                        ? state->semanticMetadataRecordCapacity * 2u
                        : 4u;
    FisicsSemanticMetadataRecord* grown = (FisicsSemanticMetadataRecord*)realloc(
        state->semanticMetadataRecords, newCap * sizeof(*grown));
    if (!grown) return false;
    state->semanticMetadataRecords = grown;
    state->semanticMetadataRecordCapacity = newCap;
    return true;
}

static FisicsSemanticMetadataRecord* find_record(FisicsExtensionState* state, Symbol* symbol) {
    for (size_t i = 0; i < state->semanticMetadataRecordCount; ++i) {
        if (state->semanticMetadataRecords[i].symbol == symbol) {
            return &state->semanticMetadataRecords[i];
        }
    }
    return NULL;
}

static FisicsSemanticMetadataRecord* ensure_record(CompilerContext* ctx,
                                                   Symbol* symbol,
                                                   const ASTNode* node,
                                                   const char* symbolName,
                                                   bool isFunction) {
    if (!fisics_extension_state_ensure(ctx)) return NULL;
    FisicsExtensionState* state = ctx->extensionState;
    FisicsSemanticMetadataRecord* record = find_record(state, symbol);
    if (record) return record;
    if (!record_reserve(state)) return NULL;
    record = &state->semanticMetadataRecords[state->semanticMetadataRecordCount++];
    memset(record, 0, sizeof(*record));
    record->symbol = symbol;
    record->symbolName = strdup(symbolName ? symbolName : "");
    record->isFunction = isFunction;
    record->location = node_loc(node);
    return record;
}

static bool edge_reserve(FisicsExtensionState* state) {
    if (state->semanticMetadataEdgeCount < state->semanticMetadataEdgeCapacity) return true;
    size_t newCapacity = state->semanticMetadataEdgeCapacity
                             ? state->semanticMetadataEdgeCapacity * 2u
                             : 4u;
    FisicsSemanticMetadataEdge* grown = (FisicsSemanticMetadataEdge*)realloc(
        state->semanticMetadataEdges, newCapacity * sizeof(*grown));
    if (!grown) return false;
    state->semanticMetadataEdges = grown;
    state->semanticMetadataEdgeCapacity = newCapacity;
    return true;
}

static FisicsSemanticMetadataEdge* find_edge(FisicsExtensionState* state,
                                             Symbol* sourceSymbol,
                                             FisicsSemanticMetadataEdgeKind kind,
                                             const char* targetId) {
    for (size_t i = 0; i < state->semanticMetadataEdgeCount; ++i) {
        FisicsSemanticMetadataEdge* edge = &state->semanticMetadataEdges[i];
        if (edge->sourceSymbol == sourceSymbol && edge->kind == kind &&
            strcmp(edge->targetId, targetId) == 0) {
            return edge;
        }
    }
    return NULL;
}

static void merge_relationship(FisicsExtensionState* state,
                               Symbol* sourceSymbol,
                               const FisicsSemanticMetadataRecord* record,
                               SourceRange occurrenceLocation,
                               OwnerRelationship* incoming) {
    if (!incoming || !incoming->targetId) return;
    if (find_edge(state, sourceSymbol, incoming->kind, incoming->targetId)) {
        free(incoming->targetId);
        incoming->targetId = NULL;
        return;
    }
    if (!edge_reserve(state)) return;
    FisicsSemanticMetadataEdge* edge =
        &state->semanticMetadataEdges[state->semanticMetadataEdgeCount++];
    memset(edge, 0, sizeof(*edge));
    edge->sourceSymbol = sourceSymbol;
    edge->sourceSymbolName = strdup(record->symbolName ? record->symbolName : "");
    edge->sourceIsFunction = record->isFunction;
    edge->sourceLocation = record->location;
    edge->kind = incoming->kind;
    edge->targetId = incoming->targetId;
    incoming->targetId = NULL;
    edge->resolution = FISICS_METADATA_REFERENCE_EXTERNAL;
    edge->location = occurrenceLocation;
}

static bool value_valid(FisicsSemanticMetadataAttrKind kind,
                        const char* value,
                        const char** outDetail) {
    if (!fisics_semantic_metadata_is_canonical_name(value, outDetail)) return false;
    const FisicsSemanticMetadataDescriptor* descriptor =
        fisics_semantic_metadata_descriptor(kind);
    if (descriptor &&
        descriptor->valuePolicy == FISICS_METADATA_VALUE_REGISTERED_DOMAIN &&
        !fisics_semantic_metadata_domain_known(value)) {
        if (outDetail) *outDetail = "domain is not present in the initial canonical domain registry";
        return false;
    }
    if (descriptor &&
        descriptor->valuePolicy == FISICS_METADATA_VALUE_REGISTERED_ROLE &&
        !fisics_semantic_metadata_role_known(value)) {
        if (outDetail) *outDetail = "role is not present in the initial canonical role registry";
        return false;
    }
    return true;
}

static bool owner_relationship_reserve(OwnerMetadata* owner) {
    if (owner->relationshipCount < owner->relationshipCapacity) return true;
    size_t newCapacity = owner->relationshipCapacity
                             ? owner->relationshipCapacity * 2u
                             : 4u;
    OwnerRelationship* grown = (OwnerRelationship*)realloc(
        owner->relationships, newCapacity * sizeof(*grown));
    if (!grown) return false;
    owner->relationships = grown;
    owner->relationshipCapacity = newCapacity;
    return true;
}

static bool owner_has_relationship(const OwnerMetadata* owner,
                                   FisicsSemanticMetadataEdgeKind kind,
                                   const char* targetId) {
    for (size_t i = 0; i < owner->relationshipCount; ++i) {
        if (owner->relationships[i].kind == kind &&
            strcmp(owner->relationships[i].targetId, targetId) == 0) {
            return true;
        }
    }
    return false;
}

static void owner_metadata_destroy(OwnerMetadata* owner) {
    if (!owner) return;
    for (size_t i = 0; i < 3; ++i) free(owner->singularValues[i]);
    for (size_t i = 0; i < owner->relationshipCount; ++i) {
        free(owner->relationships[i].targetId);
    }
    free(owner->relationships);
    memset(owner, 0, sizeof(*owner));
}

static void collect_owner_metadata(CompilerContext* ctx,
                                   ASTNode* node,
                                   OwnerMetadata* owner) {
    memset(owner, 0, sizeof(*owner));
    for (size_t i = 0; i < node->attributeCount; ++i) {
        ASTAttribute* attr = node->attributes[i];
        FisicsSemanticMetadataAttrKind kind = fisics_semantic_metadata_attribute_kind(attr);
        if (kind == FISICS_METADATA_ATTR_NONE) continue;
        const FisicsSemanticMetadataDescriptor* descriptor =
            fisics_semantic_metadata_descriptor(kind);
        if (descriptor && !descriptor->relationship) {
            owner->singularCounts[kind_index(kind)]++;
        }
        char* value = NULL;
        const char* detail = NULL;
        if (!fisics_semantic_metadata_extract_string(attr, kind, &value, &detail) ||
            !value_valid(kind, value, &detail)) {
            diag_malformed(ctx, node, kind_name(kind), detail ? detail : "invalid metadata value");
            free(value);
            continue;
        }
        if (descriptor && descriptor->relationship) {
            FisicsSemanticMetadataEdgeKind edgeKind = relationship_edge_kind(kind);
            if (owner_has_relationship(owner, edgeKind, value)) {
                diag_duplicate_relationship(ctx, node, kind_name(kind), value);
                free(value);
                continue;
            }
            if (!owner_relationship_reserve(owner)) {
                free(value);
                continue;
            }
            OwnerRelationship* relationship =
                &owner->relationships[owner->relationshipCount++];
            relationship->kind = edgeKind;
            relationship->targetId = value;
            continue;
        }
        size_t index = kind_index(kind);
        if (!owner->singularValues[index]) owner->singularValues[index] = value;
        else free(value);
    }
    for (size_t i = 0; i < 3; ++i) {
        if (owner->singularCounts[i] > 1) {
            diag_duplicate_owner(ctx, node, kind_name((FisicsSemanticMetadataAttrKind)(i + 1)));
            free(owner->singularValues[i]);
            owner->singularValues[i] = NULL;
        }
    }
}

static bool has_metadata_attrs(const ASTNode* node) {
    if (!node) return false;
    for (size_t i = 0; i < node->attributeCount; ++i) {
        if (fisics_semantic_metadata_attribute_kind(node->attributes[i]) != FISICS_METADATA_ATTR_NONE) {
            return true;
        }
    }
    return false;
}

static void diagnose_disabled_attrs(CompilerContext* ctx, ASTNode* node) {
    for (size_t i = 0; i < node->attributeCount; ++i) {
        FisicsSemanticMetadataAttrKind kind =
            fisics_semantic_metadata_attribute_kind(node->attributes[i]);
        if (kind != FISICS_METADATA_ATTR_NONE) diag_disabled(ctx, node, kind_name(kind));
    }
}

static void diagnose_invalid_target_attrs(CompilerContext* ctx, ASTNode* node) {
    for (size_t i = 0; i < node->attributeCount; ++i) {
        FisicsSemanticMetadataAttrKind kind =
            fisics_semantic_metadata_attribute_kind(node->attributes[i]);
        if (kind != FISICS_METADATA_ATTR_NONE) diag_invalid_target(ctx, node, kind_name(kind));
    }
}

static void merge_field(CompilerContext* ctx, ASTNode* node,
                        FisicsSemanticMetadataRecord* record,
                        char** destination, bool* conflicted,
                        char* incoming, const char* field) {
    if (!incoming) return;
    if (*conflicted) {
        free(incoming);
        return;
    }
    if (!*destination) {
        *destination = incoming;
        return;
    }
    if (strcmp(*destination, incoming) != 0) {
        diag_redecl_conflict(ctx, node, record->symbolName, field);
        *conflicted = true;
    }
    free(incoming);
}

static void process_supported_owner(CompilerContext* ctx, Scope* globalScope,
                                    ASTNode* node, ASTNode* nameNode,
                                    bool isFunction) {
    const char* symbolName = nameNode ? nameNode->valueNode.value : NULL;
    OwnerMetadata owner;
    collect_owner_metadata(ctx, node, &owner);
    if (!owner.singularValues[0] && !owner.singularValues[1] &&
        !owner.singularValues[2] && owner.relationshipCount == 0) {
        owner_metadata_destroy(&owner);
        return;
    }
    Symbol* symbol = lookupSymbol(&globalScope->table, symbolName);
    if (!symbol) {
        owner_metadata_destroy(&owner);
        return;
    }
    FisicsSemanticMetadataRecord* record =
        ensure_record(ctx, symbol, nameNode ? nameNode : node, symbolName, isFunction);
    if (!record) {
        owner_metadata_destroy(&owner);
        return;
    }
    merge_field(ctx, node, record, &record->authoredId, &record->idConflicted,
                owner.singularValues[0], "id");
    owner.singularValues[0] = NULL;
    merge_field(ctx, node, record, &record->domain, &record->domainConflicted,
                owner.singularValues[1], "domain");
    owner.singularValues[1] = NULL;
    merge_field(ctx, node, record, &record->role, &record->roleConflicted,
                owner.singularValues[2], "role");
    owner.singularValues[2] = NULL;
    for (size_t i = 0; i < owner.relationshipCount; ++i) {
        merge_relationship(ctx->extensionState, symbol, record, node_loc(node),
                           &owner.relationships[i]);
    }
    owner_metadata_destroy(&owner);
}

static void walk_node(CompilerContext* ctx, Scope* globalScope,
                      ASTNode* node, bool fileScope, bool overlayEnabled) {
    if (!node) return;
    bool supportedFunction = fileScope &&
        (node->type == AST_FUNCTION_DECLARATION || node->type == AST_FUNCTION_DEFINITION);
    bool supportedVariable = fileScope && node->type == AST_VARIABLE_DECLARATION &&
                             node->varDecl.varCount == 1;
    if (has_metadata_attrs(node)) {
        if (!overlayEnabled) {
            diagnose_disabled_attrs(ctx, node);
        } else if (supportedFunction) {
            ASTNode* nameNode = node->type == AST_FUNCTION_DEFINITION
                                    ? node->functionDef.funcName
                                    : node->functionDecl.funcName;
            process_supported_owner(ctx, globalScope, node,
                                    nameNode, true);
        } else if (supportedVariable) {
            ASTNode* nameNode = node->varDecl.varNames ? node->varDecl.varNames[0] : NULL;
            process_supported_owner(ctx, globalScope, node,
                                    nameNode, false);
        } else {
            diagnose_invalid_target_attrs(ctx, node);
        }
    }

    switch (node->type) {
        case AST_PROGRAM:
        case AST_BLOCK:
            for (size_t i = 0; i < node->block.statementCount; ++i) {
                walk_node(ctx, globalScope, node->block.statements[i],
                          node->type == AST_PROGRAM, overlayEnabled);
            }
            break;
        case AST_FUNCTION_DECLARATION:
            for (size_t i = 0; i < node->functionDecl.paramCount; ++i)
                walk_node(ctx, globalScope, node->functionDecl.parameters[i], false, overlayEnabled);
            break;
        case AST_FUNCTION_DEFINITION:
            for (size_t i = 0; i < node->functionDef.paramCount; ++i)
                walk_node(ctx, globalScope, node->functionDef.parameters[i], false, overlayEnabled);
            walk_node(ctx, globalScope, node->functionDef.body, false, overlayEnabled);
            break;
        case AST_STRUCT_DEFINITION:
        case AST_UNION_DEFINITION:
            for (size_t i = 0; i < node->structDef.fieldCount; ++i)
                walk_node(ctx, globalScope, node->structDef.fields[i], false, overlayEnabled);
            break;
        case AST_CONDITIONAL_DIRECTIVE:
            walk_node(ctx, globalScope, node->conditionalDirective.body, fileScope, overlayEnabled);
            break;
        case AST_IF_STATEMENT:
            walk_node(ctx, globalScope, node->ifStmt.condition, false, overlayEnabled);
            walk_node(ctx, globalScope, node->ifStmt.thenBranch, false, overlayEnabled);
            walk_node(ctx, globalScope, node->ifStmt.elseBranch, false, overlayEnabled);
            break;
        case AST_FOR_LOOP:
            walk_node(ctx, globalScope, node->forLoop.initializer, false, overlayEnabled);
            walk_node(ctx, globalScope, node->forLoop.condition, false, overlayEnabled);
            walk_node(ctx, globalScope, node->forLoop.increment, false, overlayEnabled);
            walk_node(ctx, globalScope, node->forLoop.body, false, overlayEnabled);
            break;
        case AST_WHILE_LOOP:
            walk_node(ctx, globalScope, node->whileLoop.condition, false, overlayEnabled);
            walk_node(ctx, globalScope, node->whileLoop.body, false, overlayEnabled);
            break;
        case AST_SWITCH:
            walk_node(ctx, globalScope, node->switchStmt.condition, false, overlayEnabled);
            for (size_t i = 0; i < node->switchStmt.caseListSize; ++i)
                walk_node(ctx, globalScope, node->switchStmt.caseList[i], false, overlayEnabled);
            break;
        case AST_CASE:
            walk_node(ctx, globalScope, node->caseStmt.caseValue, false, overlayEnabled);
            for (size_t i = 0; i < node->caseStmt.caseBodySize; ++i)
                walk_node(ctx, globalScope, node->caseStmt.caseBody[i], false, overlayEnabled);
            break;
        case AST_LABEL_DECLARATION:
            walk_node(ctx, globalScope, node->label.statement, false, overlayEnabled);
            break;
        case AST_STATEMENT_EXPRESSION:
            walk_node(ctx, globalScope, node->statementExpr.block, false, overlayEnabled);
            break;
        case AST_SEQUENCE:
        case AST_BINARY_EXPRESSION:
        case AST_UNARY_EXPRESSION:
        case AST_STATIC_ASSERT:
        case AST_SIZEOF:
        case AST_ALIGNOF:
            walk_node(ctx, globalScope, node->expr.left, false, overlayEnabled);
            walk_node(ctx, globalScope, node->expr.right, false, overlayEnabled);
            break;
        case AST_TERNARY_EXPRESSION:
            walk_node(ctx, globalScope, node->ternaryExpr.condition, false, overlayEnabled);
            walk_node(ctx, globalScope, node->ternaryExpr.trueExpr, false, overlayEnabled);
            walk_node(ctx, globalScope, node->ternaryExpr.falseExpr, false, overlayEnabled);
            break;
        case AST_COMMA_EXPRESSION:
            for (size_t i = 0; i < node->commaExpr.exprCount; ++i)
                walk_node(ctx, globalScope, node->commaExpr.expressions[i], false, overlayEnabled);
            break;
        case AST_CAST_EXPRESSION:
            walk_node(ctx, globalScope, node->castExpr.expression, false, overlayEnabled);
            break;
        case AST_ARRAY_ACCESS:
            walk_node(ctx, globalScope, node->arrayAccess.array, false, overlayEnabled);
            walk_node(ctx, globalScope, node->arrayAccess.index, false, overlayEnabled);
            break;
        case AST_POINTER_ACCESS:
        case AST_DOT_ACCESS:
            walk_node(ctx, globalScope, node->memberAccess.base, false, overlayEnabled);
            break;
        case AST_POINTER_DEREFERENCE:
            walk_node(ctx, globalScope, node->pointerDeref.pointer, false, overlayEnabled);
            break;
        case AST_ASSIGNMENT:
            walk_node(ctx, globalScope, node->assignment.target, false, overlayEnabled);
            walk_node(ctx, globalScope, node->assignment.value, false, overlayEnabled);
            break;
        case AST_RETURN:
            walk_node(ctx, globalScope, node->returnStmt.returnValue, false, overlayEnabled);
            break;
        case AST_FUNCTION_CALL:
            walk_node(ctx, globalScope, node->functionCall.callee, false, overlayEnabled);
            for (size_t i = 0; i < node->functionCall.argumentCount; ++i)
                walk_node(ctx, globalScope, node->functionCall.arguments[i], false, overlayEnabled);
            break;
        case AST_FUNCTION_POINTER:
            for (size_t i = 0; i < node->functionPointer.paramCount; ++i)
                walk_node(ctx, globalScope, node->functionPointer.parameters[i], false, overlayEnabled);
            walk_node(ctx, globalScope, node->functionPointer.initializer, false, overlayEnabled);
            break;
        default:
            break;
    }
}

static void validate_unique_ids(CompilerContext* ctx) {
    FisicsExtensionState* state = ctx ? ctx->extensionState : NULL;
    if (!state) return;
    for (size_t i = 0; i < state->semanticMetadataRecordCount; ++i) {
        FisicsSemanticMetadataRecord* a = &state->semanticMetadataRecords[i];
        if (!a->authoredId || a->idConflicted) continue;
        for (size_t j = 0; j < i; ++j) {
            FisicsSemanticMetadataRecord* b = &state->semanticMetadataRecords[j];
            if (!b->authoredId || b->idConflicted) continue;
            if (strcmp(a->authoredId, b->authoredId) == 0) {
                diag_duplicate_id(ctx, a);
                a->idConflicted = true;
                b->idConflicted = true;
                break;
            }
        }
    }
}

static void resolve_relationship_targets(CompilerContext* ctx) {
    FisicsExtensionState* state = ctx ? ctx->extensionState : NULL;
    if (!state) return;
    for (size_t i = 0; i < state->semanticMetadataEdgeCount; ++i) {
        FisicsSemanticMetadataEdge* edge = &state->semanticMetadataEdges[i];
        edge->resolution = FISICS_METADATA_REFERENCE_EXTERNAL;
        for (size_t j = 0; j < state->semanticMetadataRecordCount; ++j) {
            const FisicsSemanticMetadataRecord* record =
                &state->semanticMetadataRecords[j];
            if (!record->authoredId || record->idConflicted) continue;
            if (strcmp(edge->targetId, record->authoredId) == 0) {
                edge->resolution = FISICS_METADATA_REFERENCE_RESOLVED;
                break;
            }
        }
    }
}

void fisics_semantic_metadata_run(ASTNode* root, Scope* globalScope) {
    if (!root || !globalScope || !globalScope->ctx) return;
    CompilerContext* ctx = globalScope->ctx;
    bool overlayEnabled = cc_overlay_semantic_metadata_enabled(ctx);
    walk_node(ctx, globalScope, root, false, overlayEnabled);
    if (overlayEnabled) {
        validate_unique_ids(ctx);
        resolve_relationship_targets(ctx);
    }
}
