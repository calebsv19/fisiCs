#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    if (strcmp(target_unit, "bar") == 0) return value * 0.06894757293168361;
    return value;
}
#endif

struct Cell {
    double distance_m[2];
    double reserve_j;
    double pressure_bar;
};

struct GridState {
    struct Cell cells[2][2];
    double row_score[2];
};

int main(void) {
    double feet[2][2][2] = {
        {{4.0, 7.5}, {3.25, 5.5}},
        {{6.0, 2.75}, {8.25, 1.5}},
    };
    double watt_hours[2][2] = {
        {1.25, 0.75},
        {1.5, 0.5},
    };
    double psi[2][2] = {
        {15.0, 12.5},
        {18.0, 9.5},
    };
    struct GridState grid = {{{{{0.0, 0.0}, 0.0, 0.0}}}, {0.0, 0.0}};

    for (int row = 0; row < 2; ++row) {
        for (int col = 0; col < 2; ++col) {
            for (int lane = 0; lane < 2; ++lane) {
                double raw_foot
                    [[fisics::dim(length)]]
                    [[fisics::unit(foot)]] = feet[row][col][lane];
                double meters
                    [[fisics::dim(length)]]
                    [[fisics::unit(meter)]] =
                        fisics_convert_unit(raw_foot, "meter");
                grid.cells[row][col].distance_m[lane] = meters;
            }

            double reserve_wh
                [[fisics::dim(energy)]]
                [[fisics::unit(watt_hour)]] = watt_hours[row][col];
            double reserve_j
                [[fisics::dim(energy)]]
                [[fisics::unit(joule)]] =
                    fisics_convert_unit(reserve_wh, "joule");

            double raw_psi
                [[fisics::dim(pressure)]]
                [[fisics::unit(psi)]] = psi[row][col];
            double pressure_bar
                [[fisics::dim(pressure)]]
                [[fisics::unit(bar)]] =
                    fisics_convert_unit(raw_psi, "bar");

            grid.cells[row][col].reserve_j = reserve_j;
            grid.cells[row][col].pressure_bar = pressure_bar;
            grid.row_score[row] += grid.cells[row][col].distance_m[0];
            grid.row_score[row] += grid.cells[row][col].distance_m[1] * 2.0;
            grid.row_score[row] += grid.cells[row][col].reserve_j / 1000.0;
            grid.row_score[row] += grid.cells[row][col].pressure_bar * 3.0;
        }
    }

    double total = grid.row_score[0] + grid.row_score[1];
    double diagonal = grid.cells[0][0].distance_m[1] +
                      grid.cells[1][1].distance_m[0] +
                      grid.cells[0][1].reserve_j / 1000.0 +
                      grid.cells[1][0].pressure_bar;

    printf("%.6f %.6f %.6f %.6f\n",
           grid.row_score[0],
           grid.row_score[1],
           diagonal,
           total);
    return 0;
}
