#include "11__probe_wave90_function_type_typedef_direct_aggregate_contract.h"

struct Wave90AggregateResult wave90_function_type_typedef_direct_aggregate_call(
    struct Wave90AggregateResult (*callback)(struct Wave90AggregatePayload, long),
    struct Wave90AggregatePayload payload,
    long salt) {
    return callback(payload, salt);
}
