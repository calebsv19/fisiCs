#include <stdio.h>

#include "15__probe_units_runtime_multitu_checkpoint_charge_bridge_shared.h"

int main(void) {
    double speeds_fps[2] = {31.0, 44.0};
    double dt_ms[2] = {650.0, 900.0};
    double thrust_lbf[2] = {8.5, 10.25};
    double reserve_mah[2] = {1400.0, 1750.0};
    double reserve_wh[2] = {7.5, 9.25};

    ProbeCheckpointChargeState first =
        probe_checkpoint_charge_seed(speeds_fps[0], dt_ms[0]);
    first = probe_checkpoint_charge_push_force(first, thrust_lbf[0]);
    first = probe_checkpoint_charge_push_reserve(first, reserve_mah[0], reserve_wh[0]);

    ProbeCheckpointChargeState second =
        probe_checkpoint_charge_seed(speeds_fps[1], dt_ms[1]);
    second = probe_checkpoint_charge_push_force(second, thrust_lbf[1]);
    second = probe_checkpoint_charge_push_reserve(second, reserve_mah[1], reserve_wh[1]);

    ProbeCheckpointChargeState total = {
        first.distance_m + second.distance_m,
        first.energy_j + second.energy_j,
        first.charge_ah + second.charge_ah,
    };

    printf("%.6f %.6f %.6f %.6f\n",
           total.distance_m,
           total.energy_j,
           total.charge_ah,
           probe_checkpoint_charge_score(total, 18.0));
    return 0;
}
