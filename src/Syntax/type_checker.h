#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "Parser/Helpers/parsed_type.h"
#include <stdbool.h>

bool typesAreEqual(const ParsedType* a, const ParsedType* b);
bool canAssignTypes(const ParsedType* lhs, const ParsedType* rhs);

#endif // TYPE_CHECKER_H

