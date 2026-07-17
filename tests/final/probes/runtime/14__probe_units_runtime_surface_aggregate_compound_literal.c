#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value * 0.001;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    return value;
}
#endif

typedef struct UnitsCompoundLiteralState {
    double speed_mps [[fisics::dim(speed)]] [[fisics::unit(meter_per_second)]];
    double dt_s [[fisics::dim(time)]] [[fisics::unit(second)]];
    double reserve_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
} UnitsCompoundLiteralState;

static double route_state(UnitsCompoundLiteralState state) {
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = state.speed_mps * state.dt_s;
    double score
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = state.reserve_j + distance_m;
    return score;
}

int main(void) {
    double speed_a_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]] = 32.0;
    double dt_a_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]] = 2500.0;
    double reserve_a_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]] = 0.5;
    double speed_b_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]] = 18.5;
    double dt_b_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]] = 1750.0;
    double reserve_b_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]] = 0.25;
    UnitsCompoundLiteralState states[2] = {
        (UnitsCompoundLiteralState){
            .speed_mps = fisics_convert_unit(speed_a_fps, "meter_per_second"),
            .dt_s = fisics_convert_unit(dt_a_ms, "second"),
            .reserve_j = fisics_convert_unit(reserve_a_wh, "joule"),
        },
        (UnitsCompoundLiteralState){
            .speed_mps = fisics_convert_unit(speed_b_fps, "meter_per_second"),
            .dt_s = fisics_convert_unit(dt_b_ms, "second"),
            .reserve_j = fisics_convert_unit(reserve_b_wh, "joule"),
        },
    };
    UnitsCompoundLiteralState replay =
        (UnitsCompoundLiteralState){
            .speed_mps = states[0].speed_mps + states[1].speed_mps,
            .dt_s = states[0].dt_s,
            .reserve_j = states[0].reserve_j + states[1].reserve_j,
        };

    printf("%.6f %.6f %.6f %.6f\n",
           route_state(states[0]),
           route_state(states[1]),
           route_state(replay),
           replay.speed_mps * replay.dt_s);
    return 0;
}
