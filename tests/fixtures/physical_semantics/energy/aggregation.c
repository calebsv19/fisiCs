#include <fisics/extensions.h>
double energy_total(double a FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy") FISICS_ENERGY_FORM("energy.thermal"),double b FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy") FISICS_ENERGY_FORM("energy.kinetic")) FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy") FISICS_ENERGY_FORM("energy.mixed"){return a+b;}
