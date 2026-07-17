#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value / 1000.0;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    return value;
}
#endif

struct Lane {
    double speed_mps;
    double tick_s;
    double distance_m;
};

struct MatrixState {
    struct Lane lanes[2][2];
    double reserve_j[2];
};

static struct Lane make_lane(
    double speed_fps [[fisics::dim(speed)]] [[fisics::unit(foot_per_second)]],
    double tick_ms [[fisics::dim(time)]] [[fisics::unit(millisecond)]]) {
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed_fps, "meter_per_second");
    double tick_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(tick_ms, "second");
    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = speed_mps * tick_s;

    struct Lane lane = {speed_mps, tick_s, distance_m};
    return lane;
}

static struct MatrixState build_state(void) {
    double speeds[2][2] = {
        {11.0, 14.0},
        {17.5, 8.0},
    };
    double ticks[2][2] = {
        {200.0, 450.0},
        {300.0, 875.0},
    };
    double reserve_wh[2] = {2.25, 1.75};
    struct MatrixState state = {{{{0.0, 0.0, 0.0}}}, {0.0, 0.0}};

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            double speed_fps
                [[fisics::dim(speed)]]
                [[fisics::unit(foot_per_second)]] = speeds[row][col];
            double tick_ms
                [[fisics::dim(time)]]
                [[fisics::unit(millisecond)]] = ticks[row][col];
            state.lanes[row][col] = make_lane(speed_fps, tick_ms);
        }

        double reserve
            [[fisics::dim(energy)]]
            [[fisics::unit(watt_hour)]] = reserve_wh[row];
        double reserve_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] = fisics_convert_unit(reserve, "joule");
        state.reserve_j[row] = reserve_j;
    }

    return state;
}

int main(void) {
    struct MatrixState state = build_state();
    double distance_total = 0.0;
    double weighted = 0.0;

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            struct Lane lane = state.lanes[row][col];
            distance_total += lane.distance_m;
            weighted += lane.speed_mps * (double)(row + 1);
            weighted += lane.tick_s * (double)(col + 3);
            weighted += lane.distance_m * (double)(row + col + 5);
        }
        weighted += state.reserve_j[row] / (double)(row + 2);
    }

    printf("%.6f %.6f %.6f %.6f\n",
           distance_total,
           state.reserve_j[0],
           state.reserve_j[1],
           weighted);
    return 0;
}
