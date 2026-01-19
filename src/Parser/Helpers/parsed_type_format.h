#ifndef PARSED_TYPE_FORMAT_H
#define PARSED_TYPE_FORMAT_H

#include "Parser/Helpers/parsed_type.h"

// Returns a newly allocated string describing the ParsedType.
// Caller owns the returned buffer and must free it.
char* parsed_type_to_string(const ParsedType* type);

#endif // PARSED_TYPE_FORMAT_H
