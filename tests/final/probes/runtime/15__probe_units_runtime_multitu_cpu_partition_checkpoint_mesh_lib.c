#include "15__probe_units_runtime_multitu_cpu_partition_checkpoint_mesh_shared.h"

static double probe_cpu_partition_checkpoint_distance_gain(int cpu_slot) {
    static const double gains[3] = {1.0, 0.91, 1.07};
    return gains[cpu_slot % 3];
}

static double probe_cpu_partition_checkpoint_force_gain(int cpu_slot) {
    static const double gains[3] = {1.0, 1.08, 0.95};
    return gains[cpu_slot % 3];
}

ProbeCpuPartitionCheckpointState probe_cpu_partition_checkpoint_seed(
    int cpu_slot,
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double dt_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double dt_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(dt_ms, "second");
    ProbeCpuPartitionCheckpointState state = {
        (speed_mps * dt_s) * probe_cpu_partition_checkpoint_distance_gain(cpu_slot),
        0.0,
        0.0,
    };
    return state;
}

ProbeCpuPartitionCheckpointState probe_cpu_partition_checkpoint_push(
    ProbeCpuPartitionCheckpointState state,
    int cpu_slot,
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
        (thrust_n * state.distance_m) * probe_cpu_partition_checkpoint_force_gain(cpu_slot);
    state.energy_j += reserve_j;
    state.charge_ah += reserve_ah;
    return state;
}

double probe_cpu_partition_checkpoint_score(
    ProbeCpuPartitionCheckpointState state,
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
