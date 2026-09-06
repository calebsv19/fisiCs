// SPDX-License-Identifier: Apache-2.0

#include "Extensions/SemanticMetadata/semantic_metadata_parser.h"

#include <stdlib.h>
#include <string.h>

#include "AST/ast_attribute.h"

static const FisicsSemanticMetadataDescriptor kMetadataDescriptors[] = {
    {
        FISICS_METADATA_ATTR_ID, "fisics::id", "id",
        FISICS_METADATA_VALUE_CANONICAL_NAME,
        FISICS_METADATA_TARGET_FUNCTION | FISICS_METADATA_TARGET_FILE_VARIABLE,
        false, false
    },
    {
        FISICS_METADATA_ATTR_DOMAIN, "fisics::domain", "domain",
        FISICS_METADATA_VALUE_REGISTERED_DOMAIN,
        FISICS_METADATA_TARGET_FUNCTION | FISICS_METADATA_TARGET_FILE_VARIABLE,
        false, false
    },
    {
        FISICS_METADATA_ATTR_ROLE, "fisics::role", "role",
        FISICS_METADATA_VALUE_REGISTERED_ROLE,
        FISICS_METADATA_TARGET_FUNCTION | FISICS_METADATA_TARGET_FILE_VARIABLE,
        false, false
    },
    {
        FISICS_METADATA_ATTR_PRODUCES, "fisics::produces", "produces",
        FISICS_METADATA_VALUE_CANONICAL_NAME,
        FISICS_METADATA_TARGET_FUNCTION | FISICS_METADATA_TARGET_FILE_VARIABLE,
        true, true
    },
    {
        FISICS_METADATA_ATTR_CONSUMES, "fisics::consumes", "consumes",
        FISICS_METADATA_VALUE_CANONICAL_NAME,
        FISICS_METADATA_TARGET_FUNCTION | FISICS_METADATA_TARGET_FILE_VARIABLE,
        true, true
    }
};

const FisicsSemanticMetadataDescriptor* fisics_semantic_metadata_descriptors(
    size_t* outCount) {
    if (outCount) {
        *outCount = sizeof(kMetadataDescriptors) / sizeof(kMetadataDescriptors[0]);
    }
    return kMetadataDescriptors;
}

const FisicsSemanticMetadataDescriptor* fisics_semantic_metadata_descriptor(
    FisicsSemanticMetadataAttrKind kind) {
    size_t count = 0;
    const FisicsSemanticMetadataDescriptor* descriptors =
        fisics_semantic_metadata_descriptors(&count);
    for (size_t i = 0; i < count; ++i) {
        if (descriptors[i].kind == kind) return &descriptors[i];
    }
    return NULL;
}

FisicsSemanticMetadataAttrKind fisics_semantic_metadata_attribute_kind(
    const ASTAttribute* attr) {
    if (!attr || attr->syntax != AST_ATTRIBUTE_SYNTAX_CXX || !attr->payload) {
        return FISICS_METADATA_ATTR_NONE;
    }
    size_t count = 0;
    const FisicsSemanticMetadataDescriptor* descriptors =
        fisics_semantic_metadata_descriptors(&count);
    for (size_t i = 0; i < count; ++i) {
        size_t nameLen = strlen(descriptors[i].sourceName);
        if (strncmp(attr->payload, descriptors[i].sourceName, nameLen) == 0 &&
            (attr->payload[nameLen] == '\0' || attr->payload[nameLen] == '(' ||
             attr->payload[nameLen] == ' ' || attr->payload[nameLen] == '\t')) {
            return descriptors[i].kind;
        }
    }
    return FISICS_METADATA_ATTR_NONE;
}

static void skip_space(const char** cursor) {
    while (**cursor == ' ' || **cursor == '\t' || **cursor == '\n' || **cursor == '\r') {
        ++*cursor;
    }
}

bool fisics_semantic_metadata_extract_string(const ASTAttribute* attr,
                                             FisicsSemanticMetadataAttrKind kind,
                                             char** outValue,
                                             const char** outDetail) {
    if (outValue) *outValue = NULL;
    if (outDetail) *outDetail = NULL;
    const FisicsSemanticMetadataDescriptor* descriptor =
        fisics_semantic_metadata_descriptor(kind);
    const char* name = descriptor ? descriptor->sourceName : NULL;
    if (!attr || !attr->payload || !name || !outValue) return false;

    const char* p = attr->payload + strlen(name);
    skip_space(&p);
    if (*p++ != '(') {
        if (outDetail) *outDetail = "expected '(' after the metadata attribute name";
        return false;
    }
    skip_space(&p);
    if (*p++ != '"') {
        if (outDetail) *outDetail = "expected exactly one string argument";
        return false;
    }
    const char* start = p;
    while (*p && *p != '"') {
        if (*p == '\\') {
            if (outDetail) *outDetail = "escaped characters are not accepted in canonical metadata names";
            return false;
        }
        ++p;
    }
    if (*p != '"') {
        if (outDetail) *outDetail = "unterminated metadata string argument";
        return false;
    }
    size_t len = (size_t)(p - start);
    ++p;
    skip_space(&p);
    if (*p++ != ')') {
        if (outDetail) *outDetail = "expected exactly one string argument";
        return false;
    }
    skip_space(&p);
    if (*p != '\0') {
        if (outDetail) *outDetail = "unexpected tokens after metadata string argument";
        return false;
    }
    char* value = (char*)malloc(len + 1);
    if (!value) {
        if (outDetail) *outDetail = "out of memory";
        return false;
    }
    memcpy(value, start, len);
    value[len] = '\0';
    *outValue = value;
    return true;
}

bool fisics_semantic_metadata_is_canonical_name(const char* value,
                                                const char** outDetail) {
    if (outDetail) *outDetail = NULL;
    if (!value || !value[0]) {
        if (outDetail) *outDetail = "metadata names must not be empty";
        return false;
    }
    size_t len = strlen(value);
    if (len > 255) {
        if (outDetail) *outDetail = "metadata names must be at most 255 ASCII bytes";
        return false;
    }
    bool segmentStart = true;
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)value[i];
        if (c >= 128) {
            if (outDetail) *outDetail = "metadata names must contain ASCII characters only";
            return false;
        }
        if (c == '.') {
            if (segmentStart || i + 1 == len) {
                if (outDetail) *outDetail = "metadata names must not contain empty dot-separated segments";
                return false;
            }
            segmentStart = true;
            continue;
        }
        if (segmentStart) {
            if (c < 'a' || c > 'z') {
                if (outDetail) *outDetail = "each metadata name segment must begin with a lowercase ASCII letter";
                return false;
            }
            segmentStart = false;
        } else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            if (outDetail) *outDetail = "metadata names may contain lowercase ASCII letters, digits, underscores, and separating dots";
            return false;
        }
    }
    return true;
}

static bool string_in_registry(const char* value, const char* const* registry, size_t count) {
    if (!value) return false;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(value, registry[i]) == 0) return true;
    }
    return false;
}

bool fisics_semantic_metadata_domain_known(const char* value) {
    static const char* const domains[] = {
        "control", "data_io", "hardware", "physics",
        "rendering", "simulation", "storage", "thermal"
    };
    return string_in_registry(value, domains, sizeof(domains) / sizeof(domains[0]));
}

bool fisics_semantic_metadata_role_known(const char* value) {
    static const char* const roles[] = {
        "control.decide", "data.read", "data.write", "driver.initialize",
        "persistence.read", "persistence.write", "render.integrator",
        "sensor.read", "simulation.integrator", "telemetry.publish"
    };
    return string_in_registry(value, roles, sizeof(roles) / sizeof(roles[0]));
}
