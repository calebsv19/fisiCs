// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum FisicsJsonType {
    FISICS_JSON_NULL = 0,
    FISICS_JSON_BOOL,
    FISICS_JSON_NUMBER,
    FISICS_JSON_STRING,
    FISICS_JSON_ARRAY,
    FISICS_JSON_OBJECT
} FisicsJsonType;

typedef struct FisicsJsonValue FisicsJsonValue;

typedef struct FisicsJsonMember {
    char* key;
    FisicsJsonValue* value;
} FisicsJsonMember;

struct FisicsJsonValue {
    FisicsJsonType type;
    union {
        bool boolean;
        char* text;
        struct {
            FisicsJsonValue** items;
            size_t count;
        } array;
        struct {
            FisicsJsonMember* members;
            size_t count;
        } object;
    } as;
};

typedef struct FisicsJsonDiagnostic {
    char message[256];
} FisicsJsonDiagnostic;

bool fisics_json_parse(const char* text,
                       size_t length,
                       FisicsJsonValue** out_value,
                       FisicsJsonDiagnostic* diagnostic);

void fisics_json_free(FisicsJsonValue* value);

const FisicsJsonValue* fisics_json_object_get(const FisicsJsonValue* object,
                                               const char* key);

bool fisics_json_integer_u64(const FisicsJsonValue* value, uint64_t* out);
bool fisics_json_integer_size(const FisicsJsonValue* value, size_t* out);
bool fisics_json_integer_int(const FisicsJsonValue* value, int* out);
