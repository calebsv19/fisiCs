#include <stdio.h>

#include "11__probe_wave90_function_type_typedef_direct_aggregate_contract.h"

static struct Wave90AggregateResult wave90_make_aggregate(
    struct Wave90AggregatePayload payload,
    long salt) {
    struct Wave90AggregateResult result = {
        payload.first + salt,
        payload.second + payload.third,
        payload.first * payload.third - salt,
        payload.third + 1
    };
    return result;
}

int main(void) {
    struct Wave90AggregatePayload payload = {20, 3.5, 4};
    struct Wave90AggregateResult result =
        wave90_function_type_typedef_direct_aggregate_call(
            wave90_make_aggregate,
            payload,
            6);
    printf("%ld %.1f %ld %d\n",
           result.first,
           result.second,
           result.third,
           result.stamp);
    return 0;
}
