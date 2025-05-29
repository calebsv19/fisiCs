#include "type_checker.h"
#include <string.h>

// For now, basic equality: same primitive, pointer depth, and qualifiers
bool typesAreEqual(const ParsedType* a, const ParsedType* b) {
    if (!a || !b) return false;

    if (a->kind != b->kind) return false;
    if (a->primitiveType != b->primitiveType) return false;
    if (a->pointerDepth != b->pointerDepth) return false;

    if (a->kind == TYPE_USER_DEFINED) {
        if (!a->userTypeName || !b->userTypeName) return false;
        if (strcmp(a->userTypeName, b->userTypeName) != 0) return false;
    }

    // For now we skip qualifiers (const, volatile, etc.) and storage class
    return true;
}

// Allow assignment if types are identical, or compatible pointer depths
bool canAssignTypes(const ParsedType* lhs, const ParsedType* rhs) {
    if (typesAreEqual(lhs, rhs)) return true;

    // Allow assigning void* to other pointer types or vice versa
    if ((lhs->pointerDepth > 0 && rhs->pointerDepth > 0) &&
        (lhs->primitiveType == TOKEN_VOID || rhs->primitiveType == TOKEN_VOID)) {
        return true;
    }

    // Could allow numeric conversions later (e.g., int -> float)
    return false;
}

