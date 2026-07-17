#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "joule") == 0) return value * 3600.0;
    return value;
}
#endif

typedef struct UnitSample {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double force_n [[fisics::dim(force)]] [[fisics::unit(newton)]];
    double reserve_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
} UnitSample;

typedef struct UnitMatrix {
    UnitSample sample[2];
    double bias_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
} UnitMatrix;

static UnitSample make_sample(
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]],
    double force_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    UnitSample sample;
    sample.distance_m = fisics_convert_unit(distance_ft, "meter");
    sample.force_n = fisics_convert_unit(force_lbf, "newton");
    sample.reserve_j = fisics_convert_unit(reserve_wh, "joule");
    return sample;
}

static UnitMatrix rotate_matrix(UnitMatrix matrix, double extra_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    UnitMatrix out = matrix;
    UnitSample hold = out.sample[0];

    out.sample[0] = out.sample[1];
    out.sample[1] = hold;
    out.sample[0].distance_m = out.sample[0].distance_m + out.sample[1].distance_m;
    out.sample[1].reserve_j = out.sample[1].reserve_j + fisics_convert_unit(extra_wh, "joule");
    out.bias_j = out.bias_j + out.sample[0].force_n * out.sample[0].distance_m;
    return out;
}

int main(void) {
    UnitMatrix matrix;
    double bias_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]] = 0.25;
    double total_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];

    matrix.sample[0] = make_sample(10.0, 2.25, 0.50);
    matrix.sample[1] = make_sample(16.0, 3.75, 0.75);
    matrix.bias_j = fisics_convert_unit(bias_wh, "joule");

    matrix = rotate_matrix(rotate_matrix(matrix, 0.125), 0.375);
    total_j = matrix.bias_j +
              matrix.sample[0].force_n * matrix.sample[0].distance_m +
              matrix.sample[1].force_n * matrix.sample[1].distance_m +
              matrix.sample[0].reserve_j +
              matrix.sample[1].reserve_j;

    printf("%.6f %.6f %.6f %.6f %.6f\n",
           matrix.sample[0].distance_m,
           matrix.sample[1].force_n,
           matrix.sample[1].reserve_j,
           matrix.bias_j,
           total_j);
    return 0;
}
