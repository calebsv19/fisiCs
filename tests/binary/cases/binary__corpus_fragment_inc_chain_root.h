#ifndef BINARY_CORPUS_FRAGMENT_INC_CHAIN_ROOT_H
#define BINARY_CORPUS_FRAGMENT_INC_CHAIN_ROOT_H

#include "binary__corpus_fragment_inc_chain_mid.h"

static TokenInfo token_info_make(TokenKind kind, unsigned extra_flags) {
    TokenInfo t;
    t.kind = (unsigned)kind;
    t.flags = TOKEN_ALL_MASK | extra_flags;
    return t;
}

#endif
