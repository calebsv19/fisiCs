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

typedef struct UnitsAggregateReturnSample {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double force_n [[fisics::dim(force)]] [[fisics::unit(newton)]];
    double reserve_j [[fisics::dim(energy)]] [[fisics::unit(joule)]];
} UnitsAggregateReturnSample;

static UnitsAggregateReturnSample build_sample(
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]],
    double force_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double reserve_wh [[fisics::dim(energy)]] [[fisics::unit(watt_hour)]]) {
    UnitsAggregateReturnSample sample = {
        .distance_m = fisics_convert_unit(distance_ft, "meter"),
        .force_n = fisics_convert_unit(force_lbf, "newton"),
        .reserve_j = fisics_convert_unit(reserve_wh, "joule"),
    };
    return sample;
}

static double score_sample(UnitsAggregateReturnSample sample) {
    double work_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = sample.force_n * sample.distance_m;
    double total_j
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = work_j + sample.reserve_j;
    return total_j;
}

int main(void) {
    UnitsAggregateReturnSample a = build_sample(42.0, 8.5, 1.25);
    UnitsAggregateReturnSample b = build_sample(27.0, 5.0, 0.75);
    printf("%.6f %.6f %.6f %.6f\n",
           a.distance_m + b.distance_m,
           a.force_n + b.force_n,
           a.reserve_j + b.reserve_j,
           score_sample(a) + score_sample(b));
    return 0;
}
