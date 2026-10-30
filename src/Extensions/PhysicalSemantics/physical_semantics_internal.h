// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/PhysicalSemantics/physical_semantics.h"
#include "Parser/Helpers/parsed_type.h"
#define PHYSICAL_EXPR 0u
#define PHYSICAL_CALL 1u
#define PHYSICAL_DECL 2u
#define PHYSICAL_MAX_FACTS 1048576u
void *physical_state_grow(CompilerContext *,void *,size_t);
FisicsPhysicalState *physical_state(const CompilerContext *);
PhysicalFact physical_unknown(const ASTNode *, const char *);
bool physical_known(PhysicalFact);
void physical_error(CompilerContext *, const ASTNode *, int, const char *);
void physical_mismatch(CompilerContext *, const ASTNode *, int, const char *,
                       PhysicalFact, PhysicalFact);
bool physical_put(CompilerContext *, const ASTNode *, unsigned, size_t,
                  PhysicalFact);
PhysicalFact physical_get(const CompilerContext *, const ASTNode *, unsigned,
                          size_t);
PhysicalFact physical_type(CompilerContext *, const ParsedType *,
                           const ASTNode *, bool diagnose, bool scalar_result);
void physical_validate_type(CompilerContext *, const ParsedType *,
                            const ASTNode *, bool unsupported, unsigned depth);
bool physical_constructor_validate(CompilerContext *, ASTNode *);
bool physical_constructor_call(CompilerContext *, ASTNode *, PhysicalFact *);
bool physical_constructor_return(CompilerContext *, ASTNode *, ASTNode *);
void physical_function_attributes(CompilerContext *, ASTNode *);
PhysicalFact physical_boundary(CompilerContext *, const ASTNode *, const char *,
                               PhysicalFact, PhysicalFact,
                               bool literal_allowed);
void physical_event(CompilerContext *, const ASTNode *, const char *,
                    PhysicalFact, PhysicalFact, PhysicalFact);
PhysicalFact physical_expression(CompilerContext *, ASTNode *);
void physical_walk(CompilerContext *, ASTNode *, ASTNode *, unsigned);
void physical_initializer(CompilerContext *, const ParsedType *, ASTNode *,
                          ASTNode *, unsigned);
void physical_callable_assignment(CompilerContext *, const ParsedType *,
                                  ASTNode *, ASTNode *);

PhysicalFact physical_merge_trust(PhysicalFact, PhysicalFact);
/* Known scalar + or - with at least one absolute operand. */
PhysicalFact physical_affine_binary(CompilerContext *, const ASTNode *,
                                    const char *, PhysicalFact, PhysicalFact);

bool physical_raw_pointer(CompilerContext *,const ASTNode *);
bool physical_coverage_interface(CompilerContext *,const ASTNode *);

/* Only the Units layer's captured direct-callee identity can select these. */
bool physical_math_recognized(CompilerContext *,const ASTNode *);
bool physical_math_expression(CompilerContext *,ASTNode *,PhysicalFact *);

PhysicalFact physical_scalar_expression(CompilerContext *, ASTNode *, const char *,
                                        PhysicalFact, PhysicalFact);

void physical_snapshot_prepare(CompilerContext *,ASTNode *);
bool physical_snapshot_access(CompilerContext *,ASTNode *,PhysicalFact *);
bool physical_snapshot_compound(CompilerContext *,ASTNode *,PhysicalFact *);

void physical_mutable_prepare(CompilerContext *,ASTNode *);
bool physical_mutable_value(CompilerContext *,ASTNode *,PhysicalFact *);
bool physical_mutable_first_write(CompilerContext *,ASTNode *,PhysicalFact *);
bool physical_mutable_store(CompilerContext *,ASTNode *);
