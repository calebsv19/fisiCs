// SPDX-License-Identifier: Apache-2.0
#include "Extensions/Units/units_return_semantics.h"
#include "AST/ast_node.h"
#include "Compiler/compiler_context.h"
#include "Extensions/extension_diagnostics.h"
#include "Extensions/extension_units_call_contracts.h"
#include "Extensions/extension_units_expr_table.h"
#include "Extensions/Units/units_literal_semantics.h"
#include "Extensions/Units/units_parser.h"
#include <stdlib.h>

static int units_attribute_kind(ASTAttribute* attr) {
    char* text=NULL;
    if (fisics_units_dim_attribute_extract(attr,&text)) { free(text); return 1; }
    if (fisics_units_unit_attribute_extract(attr,&text)) { free(text); return 2; }
    return 0;
}

static bool has_kind(const ParsedType* type,int kind) {
    if (!type) return false;
    for (size_t i=0;i<type->attributeCount;++i)
        if (units_attribute_kind(type->attributes[i])==kind) return true;
    return false;
}

static void copy_attribute(ParsedType* type, ASTAttribute* attr) {
    ASTAttribute* copy=astAttributeClone(attr);
    if (!copy) return;
    ASTAttribute** extras=malloc(sizeof(*extras));
    if (!extras) { astAttributeDestroy(copy); return; }
    extras[0]=copy;
    (void)astAttributeListAppend(&type->attributes,&type->attributeCount,extras,1);
}

void fisics_units_inherit_return_attributes(ParsedType* current, const ParsedType* previous) {
    bool dim=has_kind(current,1), unit=has_kind(current,2);
    if (previous) for (size_t i=0;i<previous->attributeCount;++i) {
        int k=units_attribute_kind(previous->attributes[i]);
        if ((k==1 && !dim)||(k==2 && !unit)) copy_attribute(current,previous->attributes[i]);
    }
}

void fisics_units_note_function_return_contract(CompilerContext* ctx, ASTNode* node,
                                               const ParsedType* previous, ParsedType* current) {
    if (!ctx || !node || !current) return;
    /* Function suffix attributes belong to the return contract as well. The
     * AST already includes type attributes; copy only missing suffix kinds. */
    bool dim=has_kind(current,1), unit=has_kind(current,2);
    for (size_t i=0;i<node->attributeCount;++i) {
        int k=units_attribute_kind(node->attributes[i]);
        if ((k==1 && !dim)||(k==2 && !unit)) copy_attribute(current,node->attributes[i]);
    }
    if (!cc_overlay_physics_units_enabled(ctx)) return;
    FisicsDim8 pd,cd;
    bool pr=false,cr=false,pu=false,cu=false;
    const FisicsUnitDef *pv=NULL,*cv=NULL;
    fisics_units_parse_type_contract(previous,&pd,&pr,&pv,&pu);
    fisics_units_parse_type_contract(current,&cd,&cr,&cv,&cu);
    if (pr && cr && !fisics_dim_equal(pd,cd))
        fisics_extension_diag_units_return_contract_mismatch(ctx,node,pd,cd);
    else if (pr && cr && pu && cu && pv!=cv)
        fisics_extension_diag_units_implicit_unit_conversion(ctx,node,"return contract",cv,pv);
    /* An omitted annotation cannot erase a preceding header contract. Copy
     * only absent attribute families; unrelated C attributes remain intact. */
    fisics_units_inherit_return_attributes(current,previous);
    fisics_extension_note_units_call_arg_contract_from_param_type(ctx,node,
        FISICS_UNITS_RETURN_CONTRACT_INDEX,current);
}

void fisics_units_validate_return(CompilerContext* ctx, ASTNode* function, ASTNode* statement) {
    if (!ctx || !function || !statement) return;
    const FisicsUnitsCallArgContract* contract=fisics_extension_lookup_units_call_arg_contract(
        ctx,function,FISICS_UNITS_RETURN_CONTRACT_INDEX);
    if (!contract || !contract->resolved) return;
    ASTNode* value=statement->returnStmt.returnValue;
    if (fisics_units_is_literal_value(value)) return;
    const FisicsUnitsExprResult* result=fisics_extension_lookup_units_expr_result(ctx,value);
    if (!result || !result->resolved) return;
    if (!fisics_dim_equal(contract->dim,result->dim))
        fisics_extension_diag_units_return_dim_mismatch(ctx,value,contract->dim,result->dim);
    else if (contract->unitResolved && result->unitResolved && contract->unitDef!=result->unitDef)
        fisics_extension_diag_units_implicit_unit_conversion(ctx,value,"return",result->unitDef,contract->unitDef);
}
