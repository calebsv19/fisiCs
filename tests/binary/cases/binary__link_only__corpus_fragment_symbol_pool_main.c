#include "binary__corpus_fragment_symbol_pool.h"

int main(void) {
    (void)sym_pool_add("alpha", 5u);
    (void)sym_pool_add("beta", 4u);
    return (int)(sym_pool_find("beta", 4u) + (int)sym_pool_count());
}
