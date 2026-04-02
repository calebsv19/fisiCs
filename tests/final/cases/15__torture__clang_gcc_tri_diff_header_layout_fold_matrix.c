#include <stdio.h>
#include "15__torture__clang_gcc_tri_diff_header_layout_fold_matrix.h"

int main(void) {
    static const LayoutCell cells[] = {
        {(unsigned char)1u, (unsigned short)3u, 101u, -5},
        {(unsigned char)2u, (unsigned short)7u, 303u, 8},
        {(unsigned char)3u, (unsigned short)2u, 707u, -11},
        {(unsigned char)4u, (unsigned short)9u, 919u, 4},
        {(unsigned char)5u, (unsigned short)5u, 121u, -3},
    };
    unsigned r0 = mix_layout_rows(cells, 5u, 0x1234abcdu, 0x9e3779b9u);
    unsigned r1 = mix_layout_rows(cells + 1, 4u, 0x7f4a7c15u, 0x51f15e5u);
    unsigned out = (r0 * 33u) ^ (r1 * 17u + 19u);

    printf("%u %u\n", out, out ^ (r0 + r1 * 5u));
    return 0;
}
