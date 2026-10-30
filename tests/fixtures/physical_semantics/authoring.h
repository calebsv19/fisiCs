#include <fisics/extensions.h>
#define ENERGY FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.energy")
#define TORQUE FISICS_DIM(energy) FISICS_QUANTITY_KIND("quantity.torque")
#define ANGLE FISICS_DIM(1) FISICS_QUANTITY_KIND("quantity.angle")
#define RATIO FISICS_DIM(1) FISICS_QUANTITY_KIND("quantity.ratio")
#define POSITION FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.position") FISICS_PHYSICAL_ORIGIN("origin.world")
#define LOCAL_POSITION FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.position") FISICS_PHYSICAL_ORIGIN("origin.local")
#define DELTA FISICS_DIM(length) FISICS_QUANTITY_KIND("quantity.displacement")
#define TEMPERATURE FISICS_DIM(temperature) FISICS_QUANTITY_KIND("quantity.temperature.absolute") FISICS_PHYSICAL_ORIGIN("origin.kelvin.zero")
