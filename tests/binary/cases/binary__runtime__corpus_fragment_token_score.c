#include <stdio.h>

#include "binary__corpus_fragment_inc_chain_root.h"

int main(void) {
    TokenInfo t = token_info_make(tok_number, TOKEN_FLAG_CONST);
    printf("%u %u\n", t.kind, t.flags & 0xffu);
    return 0;
}
