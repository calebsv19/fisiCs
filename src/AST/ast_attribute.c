#include "AST/ast_attribute.h"

#include <stdlib.h>
#include <string.h>

static char* attr_strdup(const char* src) {
    if (!src) return NULL;
    size_t len = strlen(src);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, src, len + 1);
    return copy;
}

ASTAttribute* astAttributeCreate(ASTAttributeSyntax syntax, const char* payload) {
    ASTAttribute* attr = (ASTAttribute*)calloc(1, sizeof(ASTAttribute));
    if (!attr) {
        return NULL;
    }
    attr->syntax = syntax;
    attr->payload = attr_strdup(payload ? payload : "");
    if (payload && !attr->payload) {
        free(attr);
        return NULL;
    }
    return attr;
}

ASTAttribute* astAttributeClone(const ASTAttribute* attr) {
    if (!attr) return NULL;
    return astAttributeCreate(attr->syntax, attr->payload);
}

void astAttributeDestroy(ASTAttribute* attr) {
    if (!attr) return;
    free(attr->payload);
    free(attr);
}

ASTAttribute** astAttributeListClone(ASTAttribute* const* items, size_t count) {
    if (!items || count == 0) {
        return NULL;
    }
    ASTAttribute** clones = (ASTAttribute**)calloc(count, sizeof(ASTAttribute*));
    if (!clones) {
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        clones[i] = astAttributeClone(items[i]);
        if (!clones[i]) {
            astAttributeListDestroy(clones, i);
            free(clones);
            return NULL;
        }
    }
    return clones;
}

void astAttributeListDestroy(ASTAttribute** items, size_t count) {
    if (!items) return;
    for (size_t i = 0; i < count; ++i) {
        astAttributeDestroy(items[i]);
    }
}

bool astAttributeListAppend(ASTAttribute*** list,
                            size_t* count,
                            ASTAttribute** extras,
                            size_t extraCount) {
    if (!count) return false;
    if (!extras || extraCount == 0) {
        free(extras);
        return true;
    }
    size_t newCount = *count + extraCount;
    ASTAttribute** merged = (ASTAttribute**)realloc(*list, newCount * sizeof(ASTAttribute*));
    if (!merged) {
        astAttributeListDestroy(extras, extraCount);
        free(extras);
        return false;
    }
    memcpy(merged + *count, extras, extraCount * sizeof(ASTAttribute*));
    free(extras);
    *list = merged;
    *count = newCount;
    return true;
}
