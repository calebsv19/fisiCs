// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "Compiler/pipeline.h"
#include "Extensions/PhysicalSemantics/physical_coverage.h"
#include "Extensions/PhysicalSemantics/physical_provenance.h"
void physical_json_string(FILE *, const char *);
void physical_json_location(FILE *, const struct ASTNode *);
void physical_json_fact(FILE *, struct CompilerContext *, PhysicalFact);
bool physical_receipt_signatures(FILE *, struct CompilerContext *,
                                 const struct ASTNode *);
bool physical_receipt_write(const char *, const CompileResult *,
                            const CompileOptions *, const PhysicalCoverage *);
int physical_receipt_check(size_t, const char *const *);

void physical_frame_fact_write(FILE *, struct CompilerContext *, PhysicalFact);
bool physical_frame_fact_match(const FisicsJsonValue *, const FisicsPhysicalPolicy *);
bool physical_frame_signature_match(const FisicsJsonValue *, const FisicsPhysicalPolicy *);
bool physical_frame_document_match(const FisicsJsonValue *, const FisicsPhysicalPolicy *);
