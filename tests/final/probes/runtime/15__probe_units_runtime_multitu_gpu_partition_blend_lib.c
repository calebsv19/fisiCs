#include "15__probe_units_runtime_multitu_gpu_partition_blend_shared.h"

static double probe_gpu_partition_blend_distance_gain(int gpu_tile) {
    static const double gains[4] = {1.0, 0.94, 1.06, 1.11};
    return gains[gpu_tile % 4];
}

static double probe_gpu_partition_blend_force_gain(int gpu_tile) {
    static const double gains[4] = {1.0, 1.03, 0.97, 1.08};
    return gains[gpu_tile % 4];
}

ProbeGpuPartitionBlendState probe_gpu_partition_blend_seed(
    int gpu_tile,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    ProbeGpuPartitionBlendState state = {
        (speed_mps * dt_s) * probe_gpu_partition_blend_distance_gain(gpu_tile),
        0.0,
        0.0,
    };
    return state;
}

ProbeGpuPartitionBlendState probe_gpu_partition_blend_push(
    ProbeGpuPartitionBlendState state,
    int gpu_tile,
    double thrust_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]],
    double reserve_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    double thrust_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(thrust_lbf, "newton");
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve_wh, "joule");
    double reserve_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] =
            fisics_convert_unit(reserve_mah, "ampere_hour");
    state.energy_j +=
        (thrust_n * state.distance_m) * probe_gpu_partition_blend_force_gain(gpu_tile);
    state.energy_j += reserve_j;
    state.charge_ah += reserve_ah;
    return state;
}

double probe_gpu_partition_blend_score(
    ProbeGpuPartitionBlendState state,
    double window_ft [[fisics::dim(length)]] [[fisics::unit(foot)]],
    double tick_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double window_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(window_ft, "meter");
    double tick_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(tick_ms, "second");
    return (state.energy_j / tick_s) + (state.charge_ah / window_m);
}
