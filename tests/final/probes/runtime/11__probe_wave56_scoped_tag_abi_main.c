#include <stdio.h>

#include "11__probe_wave56_scoped_tag_abi_contract.h"

static Wave56ScopedTagAbiPayload wave56_scoped_tag_abi_transform(
    Wave56ScopedTagAbiPayload value,
    int salt) {
    Wave56ScopedTagAbiPayload out;

    out.lane = value.total + salt;
    out.total = value.trace - salt;
    out.trace = value.lane + value.total + salt;
    out.stamp = value.stamp + salt * 2;
    out.guard = value.guard - salt;
    return out;
}

int main(void) {
    Wave56ScopedTagAbiPayload start = wave56_scoped_tag_abi_make(7, 4);
    Wave56ScopedTagAbiPayload got = wave56_scoped_tag_abi_forward(
        start,
        wave56_scoped_tag_abi_transform,
        5);
    long long checksum = got.lane * 3
        + got.total * 5
        + got.trace * 7
        + (long long)got.stamp * 11
        + (long long)got.guard * 13;

    printf("%lld %lld %lld %d %d %lld\n",
           got.lane,
           got.total,
           got.trace,
           got.stamp,
           got.guard,
           checksum);
    return 0;
}
