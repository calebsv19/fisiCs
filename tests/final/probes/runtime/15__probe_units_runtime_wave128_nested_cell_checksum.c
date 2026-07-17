#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "meter_per_second") == 0) return value * 0.3048;
    if (strcmp(target_unit, "second") == 0) return value / 1000.0;
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "ampere_hour") == 0) return value / 1000.0;
    return value;
}
#endif

struct UnitCell {
    double motion_m[2];
    double work_j;
    double charge_ah;
};

struct UnitBlock {
    struct UnitCell cells[2][2];
    double checksum_lanes[3];
};

static struct UnitCell make_cell(double speed_fps, double tick_ms,
                                 double thrust_lbf, double reserve_wh,
                                 double draw_mah, double bias_ft) {
    double speed
        [[fisics::dim(speed)]]
        [[fisics::unit(foot_per_second)]] = speed_fps;
    double speed_mps
        [[fisics::dim(speed)]]
        [[fisics::unit(meter_per_second)]] =
            fisics_convert_unit(speed, "meter_per_second");

    double tick
        [[fisics::dim(time)]]
        [[fisics::unit(millisecond)]] = tick_ms;
    double tick_s
        [[fisics::dim(time)]]
        [[fisics::unit(second)]] = fisics_convert_unit(tick, "second");

    double bias
        [[fisics::dim(length)]]
        [[fisics::unit(foot)]] = bias_ft;
    double bias_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = fisics_convert_unit(bias, "meter");

    double force
        [[fisics::dim(force)]]
        [[fisics::unit(pound_force)]] = thrust_lbf;
    double force_n
        [[fisics::dim(force)]]
        [[fisics::unit(newton)]] = fisics_convert_unit(force, "newton");

    double reserve
        [[fisics::dim(energy)]]
        [[fisics::unit(watt_hour)]] = reserve_wh;
    double reserve_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = fisics_convert_unit(reserve, "joule");

    double draw
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = draw_mah;
    double charge_ah
        [[fisics::dim(charge)]]
        [[fisics::unit(ampere_hour)]] = fisics_convert_unit(draw, "ampere_hour");

    double distance_m
        [[fisics::dim(length)]]
        [[fisics::unit(meter)]] = speed_mps * tick_s + bias_m;

    double work_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = force_n * distance_m + reserve_j;

    struct UnitCell cell = {{distance_m, bias_m}, work_j, charge_ah};
    return cell;
}

int main(void) {
    struct UnitBlock block = {{{{{0.0, 0.0}, 0.0, 0.0}}}, {0.0, 0.0, 0.0}};
    double speeds[2][2] = {{11.0, 7.5}, {13.25, 9.75}};
    double ticks[2][2] = {{640.0, 925.0}, {480.0, 1110.0}};
    double thrusts[2][2] = {{2.5, 4.0}, {3.25, 5.5}};
    double reserves[2][2] = {{0.75, 1.25}, {0.5, 1.5}};
    double draws[2][2] = {{180.0, 210.0}, {160.0, 245.0}};
    double biases[2][2] = {{1.0, 2.25}, {1.75, 0.5}};
    int row;
    int col;

    for (row = 0; row < 2; ++row) {
        for (col = 0; col < 2; ++col) {
            struct UnitCell cell = make_cell(
                speeds[row][col],
                ticks[row][col],
                thrusts[row][col],
                reserves[row][col],
                draws[row][col],
                biases[row][col]);
            block.cells[row][col] = cell;
            block.checksum_lanes[0] += block.cells[row][col].motion_m[0] * (double)(row + 2);
            block.checksum_lanes[1] += block.cells[row][col].work_j / (double)(col + 3);
            block.checksum_lanes[2] += block.cells[row][col].charge_ah * (double)(row + col + 5);
        }
    }

    double diagonal = block.cells[0][0].motion_m[1] +
                      block.cells[1][1].motion_m[0] +
                      block.cells[1][0].work_j / 1000.0 +
                      block.cells[0][1].charge_ah * 10.0;

    printf("%.6f %.6f %.6f %.6f\n",
           block.checksum_lanes[0],
           block.checksum_lanes[1],
           block.checksum_lanes[2],
           diagonal);
    return 0;
}
