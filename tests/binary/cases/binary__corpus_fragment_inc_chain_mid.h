#ifndef BINARY_CORPUS_FRAGMENT_INC_CHAIN_MID_H
#define BINARY_CORPUS_FRAGMENT_INC_CHAIN_MID_H

#include "binary__corpus_fragment_inc_chain_leaf.h"

#define TOKEN_KIND_ENUM(name, mask) name,
typedef enum {
    TOKEN_KIND_LIST(TOKEN_KIND_ENUM)
    tok_count
} TokenKind;
#undef TOKEN_KIND_ENUM

#define TOKEN_MASK_ACC(name, mask) + (mask)
#define TOKEN_ALL_MASK (0u TOKEN_KIND_LIST(TOKEN_MASK_ACC))

#endif
