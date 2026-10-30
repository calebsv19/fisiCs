// SPDX-License-Identifier: Apache-2.0

#include "Compiler/diagnostic_metadata.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static const FisicsDiagnosticExplanation kFisicsDiagnosticExplanations[] = {
    { 4310, "Physical source annotations require the explicit overlay.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4311, "A physical kind/origin annotation is malformed or duplicated.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4312, "The annotated C carrier is outside the admitted physical scope.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4313, "A physical kind or origin is absent from the explicit policy.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4314, "A physical kind contradicts the existing Dim8 contract.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4315, "Physical redeclarations have conflicting kind/origin identities.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4316, "A physical assignment, argument, return or merge has incompatible identities.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4317, "A physical linear operation or comparison has incompatible kinds.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { 4319, "Selected physical obligations are incomplete or unverified.", "Inspect the physical receipt selection and loss locations.", "Author missing physical contracts or explicitly permit supported trust; contradictions and unknown facts cannot be waived." },
    { 4318, "A physical point has a missing or incompatible origin.", "See the source annotation and explicit physical policy.", "Keep dimensions and physical identities consistent; unknown facts are not conversions." },
    { FISICS_DIAG_CODE_PHYSICAL_POLICY_INVALID,
      "The physical semantics policy has an invalid schema or declaration.",
      "Malformed JSON fields, duplicate paths or invalid explicit policy configuration.",
      "Supply a supported policy schema with explicit catalog paths and canonical identities." },
    { FISICS_DIAG_CODE_PHYSICAL_POLICY_CATALOG,
      "A physical catalog contradicts its declared kind or unit contracts.",
      "Invalid or conflicting kind definitions, catalog fields, or catalog contract references.",
      "Resolve the catalog conflict without changing Dim8 or weakening schema checks." },
    { FISICS_DIAG_CODE_PHYSICAL_POLICY_REFERENCE,
      "A declared physical role, pair, origin or model handle is invalid.",
      "Unknown kinds, ambiguous pair roles, mismatched dimensions or an origin assigned to the wrong point kind.",
      "Declare explicit matching kind pairs and origins; names do not imply a physical role." },
    { FISICS_DIAG_CODE_PHYSICAL_POLICY_LIMIT,
      "Physical policy inputs exceed a bounded loader limit.",
      "Too many definitions, catalogs or origins, or oversized input files.",
      "Reduce the explicit input set; internal options may lower but not raise production limits." },
    { FISICS_DIAG_CODE_PHYSICAL_POLICY_ALLOCATION,
      "The physical policy model could not be allocated.",
      "Memory exhaustion or a caller-local allocation failure injection.",
      "The previous model is preserved; retry only after resolving the resource condition." },
    { FISICS_DIAG_CODE_PHYSICAL_POLICY_IO,
      "A physical policy input could not be read.",
      "A missing, inaccessible, nonregular or changing local input file.",
      "Check the explicit canonical policy/catalog paths; no automatic discovery is performed." },

    { FISICS_DIAG_CODE_EXTENSION_UNITS_MATH_ARGUMENT_DIM_MISMATCH,
      "A physical math helper received arguments with different dimensions.",
      "Both arguments of hypot or fmax have known, conflicting dimensions, including a bare dimensionless zero.",
      "Use matching physical quantities and explicitly typed zeroes; convert concrete units explicitly when needed." },
    { FISICS_DIAG_CODE_EXTENSION_UNITS_SQRT_UNSUPPORTED,
      "A physical square root cannot be represented by the supported dimension or unit rules.",
      "An input dimension has an odd exponent, or a known concrete unit has no declared square-root mapping.",
      "Use even dimension exponents and a supported unit mapping; no fractional exponents or implicit unit changes are introduced." },
    { FISICS_DIAG_CODE_EXTENSION_UNITS_RETURN_CONTRACT_MISMATCH,
      "A function redeclaration contradicts its declared physical result dimension.",
      "The resolved dimensions disagree at a function return boundary.",
      "Check the function result contract and return expression; unit conversion cannot change dimensions." },
    { FISICS_DIAG_CODE_EXTENSION_UNITS_RETURN_DIM_MISMATCH,
      "A function return expression contradicts its declared physical result dimension.",
      "The resolved dimensions disagree at a function return boundary.",
      "Check the function result contract and return expression; unit conversion cannot change dimensions." },
    {
        FISICS_DIAG_CODE_GENERIC,
        "A general compiler diagnostic was reported without a more specific code.",
        "Older diagnostic paths, fallback error handling, or an unexpected compiler state.",
        "Use the primary message and location first; if the issue is unclear, rerun with --emit-diags-json for stable metadata."
    },
    {
        FISICS_DIAG_CODE_PARSER_GENERIC,
        "The parser rejected source syntax before semantic analysis completed.",
        "Malformed declarations or statements, unsupported syntax, or earlier punctuation that changed parser state.",
        "Inspect the reported token and nearby punctuation; fixing the first parser error often removes follow-on errors."
    },
    {
        FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON,
        "The parser expected a semicolon to finish the current declaration or statement.",
        "A missing semicolon, an unterminated declaration, or a previous expression that consumed more tokens than intended.",
        "Add the missing semicolon or simplify the previous declaration until the parser can recover at the reported location."
    },
    {
        FISICS_DIAG_CODE_SEMANTIC_GENERIC,
        "Semantic analysis rejected a source construct after parsing succeeded.",
        "Type mismatches, invalid lvalues, undeclared names, duplicate declarations, or cross-translation-unit conflicts.",
        "Check the referenced declaration and expression types; use structured diagnostics when IDE/build consumers need exact ranges."
    },
    {
        FISICS_DIAG_CODE_PREPROCESSOR_GENERIC,
        "The preprocessor failed while expanding directives, includes, or macros.",
        "Missing include files, unbalanced conditionals, invalid macro calls, or unsupported preprocessor constructs.",
        "Check include search paths and macro arguments; JSON diagnostics may include include_stack or macro_trace context."
    },
    {
        FISICS_DIAG_CODE_LINK_STAGE_FAILED,
        "The driver completed compilation but the link command failed.",
        "Missing object files, unresolved symbols, incompatible libraries, or a linker invocation rejected by the host toolchain.",
        "Inspect the linker stderr and verify -L, -l, framework, and object inputs in the emitted driver/build graph data."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_ADD_DIM_MISMATCH,
        "A units extension addition used operands with incompatible dimensions.",
        "Adding values such as length plus time, or mixing a dimensioned value with a non-compatible unit expression.",
        "Convert operands to compatible units or restructure the expression so addition combines the same physical dimension."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_ARGUMENT_DIM_MISMATCH,
        "A units extension function call supplied an argument with an incompatible dimension.",
        "Passing a known physical quantity to a parameter declared with a different dimension.",
        "Check the argument and parameter annotations; unit conversion cannot change physical dimensions."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH,
        "A units extension assignment tried to store a value with an incompatible dimension.",
        "Assigning a value of one physical dimension to a variable declared with another dimension.",
        "Use an explicit supported conversion or change the destination declaration to match the assigned value's dimension."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE,
        "A units extension conversion requested incompatible source and target dimensions.",
        "Converting between dimensions that do not represent the same physical quantity.",
        "Convert only between compatible units, and check that both source and target unit declarations use the intended dimensions."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_DISABLED,
        "Semantic metadata syntax was used without enabling its opt-in overlay.",
        "A fisiCs semantic metadata attribute appeared while semantic-metadata was disabled.",
        "Enable --overlay=semantic-metadata, or remove the semantic metadata attributes."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_INVALID,
        "A semantic metadata attribute has an invalid argument or canonical value.",
        "The attribute was not exactly one string, violated canonical name grammar, or used an unregistered domain or role.",
        "Use one lowercase ASCII canonical name and select registered domain and role values."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_INVALID_TARGET,
        "A semantic metadata attribute is attached to an unsupported declaration target.",
        "The first contract only accepts functions and single-declarator file-scope variables.",
        "Move the annotation to a supported declaration or wait for a later target-expansion contract."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE,
        "One declaration owner repeats a singular semantic metadata kind.",
        "More than one id, domain, or role was attached to the same declaration node.",
        "Keep at most one attribute of each metadata kind on a declaration."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_REDECL_CONFLICT,
        "Compatible C redeclarations disagree on semantic metadata.",
        "A prototype, definition, or repeated file-scope declaration supplied a different value for the same metadata field.",
        "Use identical metadata on every redeclaration of the symbol."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE_ID,
        "Two distinct symbols share one authored semantic ID in a translation unit.",
        "Authored IDs identify semantic entities and must be translation-unit unique in the first contract.",
        "Assign a different canonical authored ID to one of the symbols."
    },
    {
        FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE_RELATIONSHIP,
        "One declaration owner repeats the same semantic relationship.",
        "The same produces or consumes target was attached more than once to one declaration node.",
        "Keep one occurrence of each relationship kind and target on a declaration."
    }
};

int fisics_diag_severity_id_from_kind(DiagKind kind) {
    switch (kind) {
        case DIAG_WARNING: return FISICS_DIAG_SEVERITY_WARNING;
        case DIAG_NOTE: return FISICS_DIAG_SEVERITY_INFO;
        case DIAG_ERROR:
        default: return FISICS_DIAG_SEVERITY_ERROR;
    }
}

const char* fisics_diag_severity_name(int severity_id) {
    switch (severity_id) {
        case FISICS_DIAG_SEVERITY_INFO: return "info";
        case FISICS_DIAG_SEVERITY_WARNING: return "warning";
        case FISICS_DIAG_SEVERITY_ERROR: return "error";
        default: return "unknown";
    }
}

int fisics_diag_category_id_from_code(int code_id) {
    if(code_id>=4310 && code_id<=4319)return FISICS_DIAG_CATEGORY_EXTENSION;
    if(code_id >= FISICS_DIAG_CODE_PHYSICAL_POLICY_INVALID && code_id <= FISICS_DIAG_CODE_PHYSICAL_POLICY_IO) return FISICS_DIAG_CATEGORY_EXTENSION;
    if (code_id >= 7200 && code_id <= 7299) {
        return FISICS_DIAG_CATEGORY_BUILD;
    }
    if (code_id >= 7100 && code_id <= 7199) {
        return FISICS_DIAG_CATEGORY_BUILD;
    }
    if (code_id >= 7000 && code_id <= 7099) {
        return FISICS_DIAG_CATEGORY_BUILD;
    }
    if (code_id >= 6000 && code_id <= 6099) {
        return FISICS_DIAG_CATEGORY_CODEGEN;
    }
    if (code_id >= 5000 && code_id <= 5099) {
        return FISICS_DIAG_CATEGORY_LEXER;
    }
    if (code_id >= FISICS_DIAG_CODE_EXTENSION_GENERIC &&
        code_id <= FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE_RELATIONSHIP) {
        return FISICS_DIAG_CATEGORY_EXTENSION;
    }
    if (code_id >= FISICS_DIAG_CODE_PREPROCESSOR_GENERIC &&
        code_id < FISICS_DIAG_CODE_EXTENSION_GENERIC) {
        return FISICS_DIAG_CATEGORY_PREPROCESSOR;
    }
    if (code_id >= FISICS_DIAG_CODE_SEMANTIC_GENERIC &&
        code_id < FISICS_DIAG_CODE_PREPROCESSOR_GENERIC) {
        return FISICS_DIAG_CATEGORY_SEMANTIC;
    }
    if (code_id >= FISICS_DIAG_CODE_PARSER_GENERIC &&
        code_id < FISICS_DIAG_CODE_SEMANTIC_GENERIC) {
        return FISICS_DIAG_CATEGORY_PARSER;
    }
    if (code_id >= FISICS_DIAG_CODE_GENERIC &&
        code_id < FISICS_DIAG_CODE_PARSER_GENERIC) {
        return FISICS_DIAG_CATEGORY_ANALYSIS;
    }
    return FISICS_DIAG_CATEGORY_UNKNOWN;
}

const char* fisics_diag_category_name(int category_id) {
    switch (category_id) {
        case FISICS_DIAG_CATEGORY_ANALYSIS: return "analysis";
        case FISICS_DIAG_CATEGORY_PARSER: return "parser";
        case FISICS_DIAG_CATEGORY_SEMANTIC: return "semantic";
        case FISICS_DIAG_CATEGORY_PREPROCESSOR: return "preprocessor";
        case FISICS_DIAG_CATEGORY_LEXER: return "lexer";
        case FISICS_DIAG_CATEGORY_CODEGEN: return "codegen";
        case FISICS_DIAG_CATEGORY_BUILD: return "build";
        case FISICS_DIAG_CATEGORY_EXTENSION: return "extension";
        case FISICS_DIAG_CATEGORY_UNKNOWN:
        default: return "unknown";
    }
}

const char* fisics_diag_code_name(int code_id) {
    switch (code_id) {
        case FISICS_DIAG_CODE_UNKNOWN: return "unknown";
        case FISICS_DIAG_CODE_GENERIC: return "generic";
        case FISICS_DIAG_CODE_PARSER_GENERIC: return "parser.generic";
        case FISICS_DIAG_CODE_PARSER_EXPECT_SEMICOLON: return "parser.expect_semicolon";
        case FISICS_DIAG_CODE_SEMANTIC_GENERIC: return "semantic.generic";
        case FISICS_DIAG_CODE_PREPROCESSOR_GENERIC: return "preprocessor.generic";
        case FISICS_DIAG_CODE_EXTENSION_GENERIC: return "extension.generic";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_DISABLED: return "extension.units.disabled";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_DIM: return "extension.units.invalid_dim";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE: return "extension.units.duplicate_dim";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_INVALID_UNIT: return "extension.units.invalid_unit";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_DUPLICATE_UNIT: return "extension.units.duplicate_unit";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_REQUIRES_DIM: return "extension.units.unit_requires_dim";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_UNIT_DIM_MISMATCH: return "extension.units.unit_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_ADD_DIM_MISMATCH: return "extension.units.add_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_SUB_DIM_MISMATCH: return "extension.units.sub_dim_mismatch";
        case 4310: return "extension.physical.disabled";
        case 4311: return "extension.physical.attribute";
        case 4312: return "extension.physical.target";
        case 4313: return "extension.physical.unknown";
        case 4314: return "extension.physical.dimension";
        case 4315: return "extension.physical.redeclaration";
        case 4316: return "extension.physical.boundary";
        case 4317: return "extension.physical.operation";
        case 4318: return "extension.physical.reference";
        case 4319: return "extension.physical.incomplete";
        case FISICS_DIAG_CODE_PHYSICAL_POLICY_INVALID: return "extension.physical_policy.invalid";
        case FISICS_DIAG_CODE_PHYSICAL_POLICY_CATALOG: return "extension.physical_policy.catalog";
        case FISICS_DIAG_CODE_PHYSICAL_POLICY_REFERENCE: return "extension.physical_policy.reference";
        case FISICS_DIAG_CODE_PHYSICAL_POLICY_LIMIT: return "extension.physical_policy.limit";
        case FISICS_DIAG_CODE_PHYSICAL_POLICY_ALLOCATION: return "extension.physical_policy.allocation";
        case FISICS_DIAG_CODE_PHYSICAL_POLICY_IO: return "extension.physical_policy.io";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_MATH_ARGUMENT_DIM_MISMATCH: return "extension.units.math_argument_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_SQRT_UNSUPPORTED: return "extension.units.sqrt_unsupported";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_RETURN_DIM_MISMATCH: return "extension.units.return_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_RETURN_CONTRACT_MISMATCH: return "extension.units.return_contract_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_ARGUMENT_DIM_MISMATCH: return "extension.units.argument_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_ASSIGN_DIM_MISMATCH: return "extension.units.assign_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_COMPARE_DIM_MISMATCH: return "extension.units.compare_dim_mismatch";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_EXPONENT_OVERFLOW: return "extension.units.exponent_overflow";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_UNSUPPORTED_OPERATION: return "extension.units.unsupported_operation";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_IMPLICIT_CONCRETE_CONVERSION: return "extension.units.implicit_concrete_conversion";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INVALID_TARGET: return "extension.units.conversion_invalid_target";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_INCOMPATIBLE: return "extension.units.conversion_incompatible";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_SOURCE_UNIT: return "extension.units.conversion_requires_source_unit";
        case FISICS_DIAG_CODE_EXTENSION_UNITS_CONVERSION_REQUIRES_FLOATING: return "extension.units.conversion_requires_floating";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_BASE: return "extension.semantic_metadata.base";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_DISABLED: return "extension.semantic_metadata.disabled";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_INVALID: return "extension.semantic_metadata.invalid";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_INVALID_TARGET: return "extension.semantic_metadata.invalid_target";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE: return "extension.semantic_metadata.duplicate";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_REDECL_CONFLICT: return "extension.semantic_metadata.redecl_conflict";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE_ID: return "extension.semantic_metadata.duplicate_id";
        case FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE_RELATIONSHIP: return "extension.semantic_metadata.duplicate_relationship";
        case FISICS_DIAG_CODE_LEXER_GENERIC: return "lexer.generic";
        case FISICS_DIAG_CODE_CODEGEN_GENERIC: return "codegen.generic";
        case FISICS_DIAG_CODE_BUILD_GENERIC: return "build.generic";
        case FISICS_DIAG_CODE_BUILD_DRIVER_GENERIC: return "build.driver_generic";
        case FISICS_DIAG_CODE_LINK_GENERIC: return "link.generic";
        case FISICS_DIAG_CODE_LINK_STAGE_FAILED: return "link.stage_failed";
        case FISICS_DIAG_CODE_BUILD_MANIFEST_GENERIC: return "build.manifest_generic";
        case FISICS_DIAG_CODE_BUILD_MANIFEST_LOAD_FAILED: return "build.manifest_load_failed";
        default: return "unknown";
    }
}

const char* fisics_diag_stage_name_from_code(int code_id) {
    if(code_id>=4310 && code_id<=4319)return "extension";
    if(code_id >= FISICS_DIAG_CODE_PHYSICAL_POLICY_INVALID && code_id <= FISICS_DIAG_CODE_PHYSICAL_POLICY_IO) return "extension";
    if (code_id >= 7200 && code_id <= 7299) {
        return "build";
    }
    if (code_id >= 7100 && code_id <= 7199) {
        return "link";
    }
    if (code_id >= 7000 && code_id <= 7099) {
        return "build";
    }
    if (code_id >= 6000 && code_id <= 6099) {
        return "codegen";
    }
    if (code_id >= 5000 && code_id <= 5099) {
        return "lex";
    }
    if (code_id >= FISICS_DIAG_CODE_EXTENSION_GENERIC &&
        code_id <= FISICS_DIAG_CODE_EXTENSION_METADATA_DUPLICATE_RELATIONSHIP) {
        return "extension";
    }
    if (code_id >= FISICS_DIAG_CODE_PREPROCESSOR_GENERIC &&
        code_id < FISICS_DIAG_CODE_EXTENSION_GENERIC) {
        return "preprocess";
    }
    if (code_id >= FISICS_DIAG_CODE_SEMANTIC_GENERIC &&
        code_id < FISICS_DIAG_CODE_PREPROCESSOR_GENERIC) {
        return "semantic";
    }
    if (code_id >= FISICS_DIAG_CODE_PARSER_GENERIC &&
        code_id < FISICS_DIAG_CODE_SEMANTIC_GENERIC) {
        return "parse";
    }
    return "unknown";
}

const FisicsDiagnosticExplanation* fisics_diag_explanations(size_t* countOut) {
    if (countOut) {
        *countOut = sizeof(kFisicsDiagnosticExplanations) /
                    sizeof(kFisicsDiagnosticExplanations[0]);
    }
    return kFisicsDiagnosticExplanations;
}

const FisicsDiagnosticExplanation* fisics_diag_explanation_by_code(int code_id) {
    size_t count = 0;
    const FisicsDiagnosticExplanation* explanations = fisics_diag_explanations(&count);
    for (size_t i = 0; i < count; ++i) {
        if (explanations[i].code_id == code_id) {
            return &explanations[i];
        }
    }
    return NULL;
}

const FisicsDiagnosticExplanation* fisics_diag_explanation_by_name(const char* code_name) {
    if (!code_name || !code_name[0]) {
        return NULL;
    }
    size_t count = 0;
    const FisicsDiagnosticExplanation* explanations = fisics_diag_explanations(&count);
    for (size_t i = 0; i < count; ++i) {
        const char* known = fisics_diag_code_name(explanations[i].code_id);
        if (known && strcmp(known, code_name) == 0) {
            return &explanations[i];
        }
    }
    return NULL;
}

const FisicsDiagnosticExplanation* fisics_diag_explanation_by_query(const char* query) {
    const FisicsDiagnosticExplanation* byName = fisics_diag_explanation_by_name(query);
    if (byName) {
        return byName;
    }
    if (!query || !query[0]) {
        return NULL;
    }
    errno = 0;
    char* end = NULL;
    long value = strtol(query, &end, 10);
    if (errno != 0 || end == query || !end || *end != '\0' ||
        value < 0 || value > 2147483647L) {
        return NULL;
    }
    return fisics_diag_explanation_by_code((int)value);
}
