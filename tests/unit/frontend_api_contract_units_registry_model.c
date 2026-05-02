#include <math.h>
#include <stdio.h>
#include <string.h>

#include "Extensions/Units/units_model.h"
#include "Extensions/Units/units_registry.h"

static int check_unit(const char* name,
                      FisicsDimFamily family,
                      const char* symbol,
                      double scale_to_canonical,
                      FisicsDim8 dim) {
    const FisicsUnitDef* unit = NULL;
    if (!fisics_unit_lookup(name, &unit) || !unit) {
        fprintf(stderr, "unit lookup failed for %s\n", name);
        return 1;
    }
    if (unit->family != family) {
        fprintf(stderr, "family mismatch for %s: got=%d expected=%d\n",
                name, (int)unit->family, (int)family);
        return 1;
    }
    if (unit->symbol == NULL || symbol == NULL || strcmp(unit->symbol, symbol) != 0) {
        fprintf(stderr, "symbol mismatch for %s: got=%s expected=%s\n",
                name, unit->symbol ? unit->symbol : "(null)", symbol ? symbol : "(null)");
        return 1;
    }
    if (!fisics_dim_equal(unit->dim, dim)) {
        fprintf(stderr, "dimension mismatch for %s\n", name);
        return 1;
    }
    if (fabs(unit->scale_to_canonical - scale_to_canonical) > 1e-12) {
        fprintf(stderr, "scale mismatch for %s: got=%.17g expected=%.17g\n",
                name, unit->scale_to_canonical, scale_to_canonical);
        return 1;
    }
    return 0;
}

int main(void) {
    FisicsUnitRegistryAudit audit = {0};
    if (!fisics_unit_registry_validate(&audit)) {
        fprintf(stderr, "built-in unit registry validation failed with %zu issue(s)\n",
                audit.issue_count);
        return 1;
    }
    if (audit.family_count < 10 || audit.unit_count < 20 || audit.alias_count < 10) {
        fprintf(stderr,
                "built-in registry audit unexpectedly small: families=%zu units=%zu aliases=%zu\n",
                audit.family_count, audit.unit_count, audit.alias_count);
        return 1;
    }

    if (fisics_unit_family_count() < 10) {
        fprintf(stderr, "unit family registry unexpectedly small: %zu\n", fisics_unit_family_count());
        return 1;
    }

    if (check_unit("meter", FISICS_DIM_FAMILY_LENGTH, "m", 1.0, fisics_dim_length())) return 1;
    if (check_unit("millimetre", FISICS_DIM_FAMILY_LENGTH, "mm", 0.001, fisics_dim_length())) return 1;
    if (check_unit("centimeters", FISICS_DIM_FAMILY_LENGTH, "cm", 0.01, fisics_dim_length())) return 1;
    if (check_unit("kilometer", FISICS_DIM_FAMILY_LENGTH, "km", 1000.0, fisics_dim_length())) return 1;
    if (check_unit("metre", FISICS_DIM_FAMILY_LENGTH, "m", 1.0, fisics_dim_length())) return 1;
    if (check_unit("yard", FISICS_DIM_FAMILY_LENGTH, "yd", 0.9144, fisics_dim_length())) return 1;
    if (check_unit("miles", FISICS_DIM_FAMILY_LENGTH, "mi", 1609.344, fisics_dim_length())) return 1;
    if (check_unit("second", FISICS_DIM_FAMILY_TIME, "s", 1.0, fisics_dim_time())) return 1;
    if (check_unit("milliseconds", FISICS_DIM_FAMILY_TIME, "ms", 0.001, fisics_dim_time())) return 1;
    if (check_unit("minutes", FISICS_DIM_FAMILY_TIME, "min", 60.0, fisics_dim_time())) return 1;
    if (check_unit("hour", FISICS_DIM_FAMILY_TIME, "h", 3600.0, fisics_dim_time())) return 1;
    if (check_unit("milligram", FISICS_DIM_FAMILY_MASS, "mg", 0.000001, fisics_dim_mass())) return 1;
    if (check_unit("tonne", FISICS_DIM_FAMILY_MASS, "t", 1000.0, fisics_dim_mass())) return 1;
    if (check_unit("pounds", FISICS_DIM_FAMILY_MASS, "lb", 0.45359237, fisics_dim_mass())) return 1;
    if (check_unit("newtons", FISICS_DIM_FAMILY_FORCE, "N", 1.0, fisics_dim_force())) return 1;
    if (check_unit("kilonewtons", FISICS_DIM_FAMILY_FORCE, "kN", 1000.0, fisics_dim_force())) return 1;
    if (check_unit("pound_force", FISICS_DIM_FAMILY_FORCE, "lbf", 4.4482216152605, fisics_dim_force())) return 1;
    if (check_unit("joule", FISICS_DIM_FAMILY_ENERGY, "J", 1.0, fisics_dim_energy())) return 1;
    if (check_unit("kilojoules", FISICS_DIM_FAMILY_ENERGY, "kJ", 1000.0, fisics_dim_energy())) return 1;
    if (check_unit("watt_hour", FISICS_DIM_FAMILY_ENERGY, "Wh", 3600.0, fisics_dim_energy())) return 1;
    if (check_unit("kilowatt_hours", FISICS_DIM_FAMILY_ENERGY, "kWh", 3600000.0, fisics_dim_energy())) return 1;
    if (check_unit("milliwatt", FISICS_DIM_FAMILY_POWER, "mW", 0.001, fisics_dim_power())) return 1;
    if (check_unit("kilowatts", FISICS_DIM_FAMILY_POWER, "kW", 1000.0, fisics_dim_power())) return 1;
    if (check_unit("megawatt", FISICS_DIM_FAMILY_POWER, "MW", 1000000.0, fisics_dim_power())) return 1;
    if (check_unit("kilopascals", FISICS_DIM_FAMILY_PRESSURE, "kPa", 1000.0, fisics_dim_pressure())) return 1;
    if (check_unit("bar", FISICS_DIM_FAMILY_PRESSURE, "bar", 100000.0, fisics_dim_pressure())) return 1;
    if (check_unit("psi", FISICS_DIM_FAMILY_PRESSURE, "psi", 6894.757293168361, fisics_dim_pressure())) return 1;
    if (check_unit("ampere_hour", FISICS_DIM_FAMILY_CHARGE, "Ah", 3600.0, fisics_dim_charge())) return 1;
    if (check_unit("milliampere_hours", FISICS_DIM_FAMILY_CHARGE, "mAh", 3.6, fisics_dim_charge())) return 1;
    if (check_unit("millivolts", FISICS_DIM_FAMILY_VOLTAGE, "mV", 0.001, fisics_dim_voltage())) return 1;
    if (check_unit("kilovolt", FISICS_DIM_FAMILY_VOLTAGE, "kV", 1000.0, fisics_dim_voltage())) return 1;
    if (check_unit("milliohm", FISICS_DIM_FAMILY_RESISTANCE, "mOhm", 0.001, fisics_dim_resistance())) return 1;
    if (check_unit("kiloohms", FISICS_DIM_FAMILY_RESISTANCE, "kOhm", 1000.0, fisics_dim_resistance())) return 1;
    if (check_unit("megaohm", FISICS_DIM_FAMILY_RESISTANCE, "MOhm", 1000000.0, fisics_dim_resistance())) return 1;
    if (check_unit("foot_per_second", FISICS_DIM_FAMILY_VELOCITY, "ft/s", 0.3048, fisics_dim_velocity())) return 1;

    const FisicsUnitFamilyDef* family = NULL;
    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_ENERGY, &family) || !family) {
        fprintf(stderr, "energy family lookup failed\n");
        return 1;
    }
    if (family->unit_count == 0 || family->units == NULL) {
        fprintf(stderr, "energy family unexpectedly empty\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_LENGTH, &family) || !family || family->unit_count < 8) {
        fprintf(stderr, "length family unexpectedly small after expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_MASS, &family) || !family || family->unit_count < 4) {
        fprintf(stderr, "mass family unexpectedly small after expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_TIME, &family) || !family || family->unit_count < 4) {
        fprintf(stderr, "time family unexpectedly small after expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_FORCE, &family) || !family || family->unit_count < 3) {
        fprintf(stderr, "force family unexpectedly small after engineering expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_ENERGY, &family) || !family || family->unit_count < 4) {
        fprintf(stderr, "energy family unexpectedly small after engineering expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_POWER, &family) || !family || family->unit_count < 4) {
        fprintf(stderr, "power family unexpectedly small after engineering expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_PRESSURE, &family) || !family || family->unit_count < 4) {
        fprintf(stderr, "pressure family unexpectedly small after engineering expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_CHARGE, &family) || !family || family->unit_count < 3) {
        fprintf(stderr, "charge family unexpectedly small after engineering expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_VOLTAGE, &family) || !family || family->unit_count < 3) {
        fprintf(stderr, "voltage family unexpectedly small after engineering expansion\n");
        return 1;
    }

    if (!fisics_unit_lookup_family(FISICS_DIM_FAMILY_RESISTANCE, &family) || !family || family->unit_count < 4) {
        fprintf(stderr, "resistance family unexpectedly small after engineering expansion\n");
        return 1;
    }

    FisicsDimAtomInfo atom = {0};
    if (!fisics_dim_lookup_atom("meter", &atom) || atom.kind != FISICS_DIM_ATOM_DEFERRED_UNIT ||
        atom.family != FISICS_DIM_FAMILY_LENGTH) {
        fprintf(stderr, "meter should remain a deferred unit-word in dim(...)\n");
        return 1;
    }

    const FisicsUnitDef* unit = NULL;
    if (fisics_unit_lookup("distance", &unit)) {
        fprintf(stderr, "dimension alias unexpectedly resolved as unit registry entry\n");
        return 1;
    }

    if (!fisics_dim_lookup_atom("millisecond", &atom) || atom.kind != FISICS_DIM_ATOM_DEFERRED_UNIT ||
        atom.family != FISICS_DIM_FAMILY_TIME) {
        fprintf(stderr, "millisecond should remain a deferred unit-word in dim(...)\n");
        return 1;
    }

    if (!fisics_dim_lookup_atom("kilowatt_hour", &atom) || atom.kind != FISICS_DIM_ATOM_DEFERRED_UNIT ||
        atom.family != FISICS_DIM_FAMILY_ENERGY) {
        fprintf(stderr, "kilowatt_hour should remain a deferred unit-word in dim(...)\n");
        return 1;
    }

    return 0;
}
