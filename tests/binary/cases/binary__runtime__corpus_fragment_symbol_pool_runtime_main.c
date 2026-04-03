#include <stdio.h>

#include "binary__corpus_fragment_symbol_pool.h"

int main(void) {
    int a = sym_pool_add("ax", 2u);
    int b = sym_pool_add("bx", 2u);
    printf("%d %d %u\n", a, b, sym_pool_count());
    return 0;
}
