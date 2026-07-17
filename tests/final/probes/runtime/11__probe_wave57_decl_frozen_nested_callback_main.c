#include <stdio.h>

#include "11__probe_wave57_decl_frozen_nested_callback_contract.h"

static Wave57DeclFrozenPayload wave57_transform_alpha(
    Wave57DeclFrozenPayload value,
    int salt) {
    Wave57DeclFrozenPayload out;

    out.x = value.y + salt;
    out.y = value.x * 2 - salt;
    out.stamp = value.stamp + salt;
    out.guard = value.guard ^ (salt + 3);
    return out;
}

static Wave57DeclFrozenPayload wave57_transform_beta(
    Wave57DeclFrozenPayload value,
    int salt) {
    Wave57DeclFrozenPayload out;

    out.x = value.x + value.y + salt;
    out.y = value.y * 3 - value.x;
    out.stamp = value.stamp - salt;
    out.guard = value.guard + salt * 2;
    return out;
}

static Wave57DeclFrozenCallback wave57_choose_transform(int route) {
    return (route & 1) ? wave57_transform_beta : wave57_transform_alpha;
}

int main(void) {
    Wave57DeclFrozenPayload start = {5, 11, 17, 23};
    Wave57DeclFrozenPayload got = wave57_decl_frozen_nested_callback_route(
        start,
        wave57_choose_transform,
        1,
        4);
    long long checksum = got.x * 3
        + got.y * 5
        + (long long)got.stamp * 7
        + (long long)got.guard * 11;

    printf("%lld %lld %d %d %lld\n",
           got.x,
           got.y,
           got.stamp,
           got.guard,
           checksum);
    return 0;
}
