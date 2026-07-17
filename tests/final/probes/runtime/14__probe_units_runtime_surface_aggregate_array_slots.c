#include <stdio.h>
#include <string.h>

#ifndef __FISICS__
static double fisics_convert_unit(double value, const char* target_unit) {
    if (strcmp(target_unit, "meter") == 0) return value * 0.3048;
    if (strcmp(target_unit, "newton") == 0) return value * 4.4482216152605;
    if (strcmp(target_unit, "coulomb") == 0) return value * 3.6;
    return value;
}
#endif

typedef struct UnitsAggregateSlot {
    double distance_m [[fisics::dim(length)]] [[fisics::unit(meter)]];
    double force_n [[fisics::dim(force)]] [[fisics::unit(newton)]];
    double charge_c [[fisics::dim(charge)]] [[fisics::unit(coulomb)]];
} UnitsAggregateSlot;

static UnitsAggregateSlot make_slot(
    double distance_ft [[fisics::dim(length)]] [[fisics::unit(foot)]],
    double force_lbf [[fisics::dim(force)]] [[fisics::unit(pound_force)]],
    double charge_mah [[fisics::dim(charge)]] [[fisics::unit(milliampere_hour)]]) {
    UnitsAggregateSlot slot;
    slot.distance_m = fisics_convert_unit(distance_ft, "meter");
    slot.force_n = fisics_convert_unit(force_lbf, "newton");
    slot.charge_c = fisics_convert_unit(charge_mah, "coulomb");
    return slot;
}

int main(void) {
    UnitsAggregateSlot slots[3];
    double extra_charge_mah
        [[fisics::dim(charge)]]
        [[fisics::unit(milliampere_hour)]] = 125.0;
    double work_sum
        [[fisics::dim(energy)]]
        [[fisics::unit(joule)]] = 0.0;
    double charge_sum
        [[fisics::dim(charge)]]
        [[fisics::unit(coulomb)]] = 0.0;
    int i;

    slots[0] = make_slot(12.0, 2.5, 500.0);
    slots[1] = make_slot(18.0, 3.0, 750.0);
    slots[2] = make_slot(7.5, 1.75, 250.0);

    slots[1].distance_m = slots[1].distance_m + slots[0].distance_m;
    slots[2] = slots[0];
    slots[2].charge_c =
        slots[2].charge_c + fisics_convert_unit(extra_charge_mah, "coulomb");

    for (i = 0; i < 3; ++i) {
        work_sum += slots[i].force_n * slots[i].distance_m;
        charge_sum += slots[i].charge_c;
    }

    printf("%.6f %.6f %.6f %.6f\n",
           slots[1].distance_m,
           slots[2].force_n,
           work_sum,
           charge_sum);
    return 0;
}
