#ifndef CODEGEN_MODEL_ADAPTER_H
#define CODEGEN_MODEL_ADAPTER_H

#include "Syntax/semantic_model.h"
#include "Syntax/semantic_model_details.h"
#include "Parser/Helpers/parsed_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CGStructField {
    const char* name;
    ParsedType type;
} CGStructField;

typedef struct CGStructInfo {
    const char* name;
    const Symbol* symbol;
    const ASTNode* definition;
    const CGStructField* fields;
    size_t fieldCount;
    bool isUnion;
} CGStructInfo;

typedef struct CGTypedefInfo {
    const char* name;
    ParsedType type;
} CGTypedefInfo;

size_t codegenModelGetStructCount(const SemanticModel* model);
void codegenModelFillStructInfo(const SemanticModel* model, CGStructInfo* buffer, size_t bufferSize);
size_t codegenModelGetTypedefCount(const SemanticModel* model);
void codegenModelFillTypedefInfo(const SemanticModel* model, CGTypedefInfo* buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif // CODEGEN_MODEL_ADAPTER_H
