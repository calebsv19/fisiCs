// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>

struct ASTAttribute;

typedef enum {
    FISICS_METADATA_ATTR_NONE = 0,
    FISICS_METADATA_ATTR_ID,
    FISICS_METADATA_ATTR_DOMAIN,
    FISICS_METADATA_ATTR_ROLE,
    FISICS_METADATA_ATTR_PRODUCES,
    FISICS_METADATA_ATTR_CONSUMES,
    FISICS_METADATA_ATTR_COUNT
} FisicsSemanticMetadataAttrKind;

typedef enum {
    FISICS_METADATA_VALUE_CANONICAL_NAME = 0,
    FISICS_METADATA_VALUE_REGISTERED_DOMAIN,
    FISICS_METADATA_VALUE_REGISTERED_ROLE
} FisicsSemanticMetadataValuePolicy;

enum {
    FISICS_METADATA_TARGET_FUNCTION = 1u << 0,
    FISICS_METADATA_TARGET_FILE_VARIABLE = 1u << 1
};

typedef struct {
    FisicsSemanticMetadataAttrKind kind;
    const char* sourceName;
    const char* displayName;
    FisicsSemanticMetadataValuePolicy valuePolicy;
    unsigned allowedTargetMask;
    bool repeatable;
    bool relationship;
} FisicsSemanticMetadataDescriptor;

const FisicsSemanticMetadataDescriptor* fisics_semantic_metadata_descriptors(
    size_t* outCount);
const FisicsSemanticMetadataDescriptor* fisics_semantic_metadata_descriptor(
    FisicsSemanticMetadataAttrKind kind);

FisicsSemanticMetadataAttrKind fisics_semantic_metadata_attribute_kind(
    const struct ASTAttribute* attr);
bool fisics_semantic_metadata_extract_string(const struct ASTAttribute* attr,
                                             FisicsSemanticMetadataAttrKind kind,
                                             char** outValue,
                                             const char** outDetail);
bool fisics_semantic_metadata_is_canonical_name(const char* value,
                                                const char** outDetail);
bool fisics_semantic_metadata_domain_known(const char* value);
bool fisics_semantic_metadata_role_known(const char* value);
