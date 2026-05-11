// SPDX-License-Identifier: Apache-2.0

#ifndef KEYWORD_LOOKUP_H
#define KEYWORD_LOOKUP_H

#include <stddef.h>

// Keyword lookup implementation for lexer classification.
const char *in_keyword_set(const char *str, size_t len);

#endif
