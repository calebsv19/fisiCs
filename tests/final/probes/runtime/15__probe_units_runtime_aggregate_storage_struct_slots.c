#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value / 1000.0;
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    return value;
}
#endif

struct MotionSlot {
    double distance_m;
    double work_j;
    double charge_ah;
};

struct RuntimeState {
    struct MotionSlot slots[3];
    double totals[3];
};

int main(void) {
    double speeds_fps[3] = {12.0, 18.5, 9.25};
    double ticks_ms[3] = {500.0, 750.0, 1250.0};
    double thrust_lbf[3] = {3.5, 4.25, 2.75};
    double draw_mah[3] = {250.0, 175.0, 325.0};
    struct RuntimeState state = {{{0.0, 0.0, 0.0}}, {0.0, 0.0, 0.0}};

    for (int i = 0; i < 3; ++i) {
        double speed_fps
            [[fisics::dim(speed)]]
            [[fisics::unit(foot_per_second)]] = speeds_fps[i];
        double speed_mps
            [[fisics::dim(speed)]]
            [[fisics::unit(meter_per_second)]] =
                fisics_convert_unit(speed_fps, "meter_per_second");

        double tick_ms
            [[fisics::dim(time)]]
            [[fisics::unit(millisecond)]] = ticks_ms[i];
        double tick_s
            [[fisics::dim(time)]]
            [[fisics::unit(second)]] = fisics_convert_unit(tick_ms, "second");

        double distance_m
            [[fisics::dim(length)]]
            [[fisics::unit(meter)]] = speed_mps * tick_s;

        double force_lbf
            [[fisics::dim(force)]]
            [[fisics::unit(pound_force)]] = thrust_lbf[i];
        double force_n
            [[fisics::dim(force)]]
            [[fisics::unit(newton)]] = fisics_convert_unit(force_lbf, "newton");

        double work_j
            [[fisics::dim(energy)]]
            [[fisics::unit(joule)]] = force_n * distance_m;

        double raw_draw
            [[fisics::dim(charge)]]
            [[fisics::unit(milliampere_hour)]] = draw_mah[i];
        double charge_ah
            [[fisics::dim(charge)]]
            [[fisics::unit(ampere_hour)]] =
                fisics_convert_unit(raw_draw, "ampere_hour");

        state.slots[i].distance_m = distance_m;
        state.slots[i].work_j = work_j;
        state.slots[i].charge_ah = charge_ah;
        state.totals[0] += state.slots[i].distance_m;
        state.totals[1] += state.slots[i].work_j;
        state.totals[2] += state.slots[i].charge_ah;
    }

    double checksum = 0.0;
    for (int i = 0; i < 3; ++i) {
        checksum += state.slots[i].distance_m * (double)(i + 3);
        checksum += state.slots[i].work_j * (double)(i + 5);
        checksum += state.slots[i].charge_ah * (double)(i + 7);
    }

    printf("%.6f %.6f %.6f %.6f\n",
           state.totals[0],
           state.totals[1],
           state.totals[2],
           checksum);
    return 0;
}
