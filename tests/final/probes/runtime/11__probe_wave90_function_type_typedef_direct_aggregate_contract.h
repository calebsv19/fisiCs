#ifndef FISICS_PROBE_WAVE90_FUNCTION_TYPE_TYPEDEF_DIRECT_AGGREGATE_CONTRACT_H
#define FISICS_PROBE_WAVE90_FUNCTION_TYPE_TYPEDEF_DIRECT_AGGREGATE_CONTRACT_H

struct Wave90AggregatePayload {
    long first;
    double second;
    int third;
};

struct Wave90AggregateResult {
    long first;
    double second;
    long third;
    int stamp;
};

typedef struct Wave90AggregateResult Wave90AggregateFunction(
    struct Wave90AggregatePayload payload,
    long salt);

struct Wave90AggregateResult wave90_function_type_typedef_direct_aggregate_call(
    Wave90AggregateFunction callback,
    struct Wave90AggregatePayload payload,
    long salt);

#endif
