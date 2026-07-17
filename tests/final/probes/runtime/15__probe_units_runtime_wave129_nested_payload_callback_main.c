#include "15__probe_units_runtime_wave129_nested_payload_callback.h"
#include <stdio.h>

static struct Wave129UnitsPayload boost_payload(struct Wave129UnitsPayload payload, double tick_s) {
    (void)tick_s;
    payload.distance_m[0] += payload.distance_m[1] * 0.25;
    payload.reserve_j += payload.reserve_j * 0.02;
    payload.charge_ah += payload.charge_ah * 0.125;
    return payload;
}

static struct Wave129UnitsPayload trim_payload(struct Wave129UnitsPayload payload, double tick_s) {
    (void)tick_s;
    payload.distance_m[1] += payload.distance_m[0] * 0.125;
    payload.reserve_j -= payload.reserve_j * 0.003;
    payload.charge_ah += payload.charge_ah * 0.0625;
    return payload;
}

int main(void) {
    Wave129UnitsCallback callbacks[2] = {boost_payload, trim_payload};
    struct Wave129UnitsPayload ring[3];
    double checksum = 0.0;
    int i;

    ring[0] = wave129_units_seed(6.5, 1.25, 225.0);
    ring[1] = wave129_units_seed(3.25, 0.75, 160.0);
    ring[2] = wave129_units_seed(9.0, 1.5, 310.0);

    for (i = 0; i < 6; ++i) {
        int src = i % 3;
        int dst = (src + 1) % 3;
        ring[dst] = wave129_units_apply(ring[src], callbacks[i & 1], 250.0 + (double)(i * 75));
        checksum += wave129_units_score(ring[dst], i + 1);
    }

    printf("%.6f %.6f %.6f\n",
           ring[0].distance_m[0] + ring[2].distance_m[1],
           ring[1].reserve_j,
           checksum);
    return 0;
}
