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
    if (fisics_unit_family_count() < 10) {
        fprintf(stderr, "unit family registry unexpectedly small: %zu\n", fisics_unit_family_count());
        return 1;
    }

    if (check_unit("meter", FISICS_DIM_FAMILY_LENGTH, "m", 1.0, fisics_dim_length())) return 1;
    if (check_unit("metre", FISICS_DIM_FAMILY_LENGTH, "m", 1.0, fisics_dim_length())) return 1;
    if (check_unit("second", FISICS_DIM_FAMILY_TIME, "s", 1.0, fisics_dim_time())) return 1;
    if (check_unit("minutes", FISICS_DIM_FAMILY_TIME, "min", 60.0, fisics_dim_time())) return 1;
    if (check_unit("joule", FISICS_DIM_FAMILY_ENERGY, "J", 1.0, fisics_dim_energy())) return 1;
    if (check_unit("newtons", FISICS_DIM_FAMILY_FORCE, "N", 1.0, fisics_dim_force())) return 1;
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

    return 0;
}
