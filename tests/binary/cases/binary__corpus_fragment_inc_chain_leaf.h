#ifndef BINARY_CORPUS_FRAGMENT_INC_CHAIN_LEAF_H
#define BINARY_CORPUS_FRAGMENT_INC_CHAIN_LEAF_H

typedef struct {
    unsigned kind;
    unsigned flags;
} TokenInfo;

#define TOKEN_KIND_LIST(X) \
    X(tok_ident, 1u)       \
    X(tok_number, 2u)      \
    X(tok_string, 4u)

#define TOKEN_FLAG_CONST 0x10u
#define TOKEN_FLAG_VOLATILE 0x20u

#endif
