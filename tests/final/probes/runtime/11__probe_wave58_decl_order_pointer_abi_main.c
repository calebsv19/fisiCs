#include <stdio.h>

#include "11__probe_wave58_decl_order_pointer_abi_contract.h"

int main(void) {
    Wave58DeclOrderPayload input = {7, 11, 13, 17};
    Wave58DeclOrderPayload shadowed = {0, 0, 0, 0};
    Wave58DeclOrderPayload alpha = {0, 0, 0, 0};
    Wave58DeclOrderPayload *shadowed_result;
    Wave58DeclOrderPayload *alpha_result;
    int same;
    long long checksum;

    shadowed_result = wave58_decl_order_pointer_shadow(&shadowed, &input, 5);
    alpha_result = wave58_decl_order_pointer_alpha(&alpha, &input, 5);
    same = shadowed.lane == alpha.lane
        && shadowed.total == alpha.total
        && shadowed.stamp == alpha.stamp
        && shadowed.guard == alpha.guard;
    checksum = shadowed.lane * 3
        + shadowed.total * 5
        + (long long)shadowed.stamp * 7
        + (long long)shadowed.guard * 11;

    printf("%d %d %d %lld %lld %d %d %lld\n",
           shadowed_result == &shadowed,
           alpha_result == &alpha,
           same,
           shadowed.lane,
           shadowed.total,
           shadowed.stamp,
           shadowed.guard,
           checksum);
    return (shadowed_result == &shadowed && alpha_result == &alpha && same) ? 0 : 1;
}
