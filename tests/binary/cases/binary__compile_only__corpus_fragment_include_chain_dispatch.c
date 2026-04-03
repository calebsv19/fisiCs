#include "binary__corpus_fragment_inc_chain_root.h"

unsigned include_chain_dispatch_score(void) {
    TokenInfo a = token_info_make(tok_ident, TOKEN_FLAG_CONST);
    TokenInfo b = token_info_make(tok_string, TOKEN_FLAG_VOLATILE);
    return a.kind + b.kind + ((a.flags ^ b.flags) & 0xffu);
}
