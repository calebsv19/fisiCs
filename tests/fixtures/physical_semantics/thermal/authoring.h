/* Compiler qualification fixtures; not maintained thermal solver adoption. */
#include <fisics/extensions.h>
#if defined(THERMAL_PHYSICAL)
#define TH_KIND(x) FISICS_QUANTITY_KIND(x)
#define TH_ORIGIN(x) FISICS_PHYSICAL_ORIGIN(x)
#define TH_FORM(x) FISICS_ENERGY_FORM(x)
#else
#define TH_KIND(x)
#define TH_ORIGIN(x)
#define TH_FORM(x)
#endif
#if defined(THERMAL_PHYSICAL) || defined(THERMAL_DIMENSIONAL)
#define TH_DIM(x) FISICS_DIM(x)
#else
#define TH_DIM(x)
#endif
#define TH_ENERGY TH_DIM(energy) TH_KIND("quantity.energy")
#define TH_TORQUE TH_DIM(energy) TH_KIND("quantity.torque")
#define TH_POSITION TH_DIM(length) TH_KIND("quantity.position") TH_ORIGIN("origin.world")
#define TH_DISPLACEMENT TH_DIM(length) TH_KIND("quantity.displacement")
#define TH_TEMPERATURE_ABSOLUTE TH_DIM(temperature) TH_KIND("quantity.temperature.absolute") TH_ORIGIN("origin.kelvin.zero")
#define TH_TEMPERATURE_DIFFERENCE TH_DIM(temperature) TH_KIND("quantity.temperature.difference")
#define TH_ANGLE TH_DIM(1) TH_KIND("quantity.angle")
#define TH_RATIO TH_DIM(1) TH_KIND("quantity.ratio")
#define TH_UNPROFILED TH_DIM(1) TH_KIND("quantity.unprofiled")
#define TH_POWER TH_DIM(energy/time) TH_KIND("quantity.power")
#define TH_DURATION TH_DIM(time) TH_KIND("quantity.duration")
#define TH_HEAT_CAPACITY TH_DIM(energy/temperature) TH_KIND("quantity.heat_capacity")
#define TH_THERMAL_CONDUCTANCE TH_DIM(energy/time/temperature) TH_KIND("quantity.thermal_conductance")
#define TH_TEMPERATURE_RATE TH_DIM(temperature/time) TH_KIND("quantity.temperature_rate")
#define TH_INVERSE_DURATION TH_DIM(1/time) TH_KIND("quantity.inverse_duration")
#define TH_THERMAL TH_ENERGY TH_FORM("energy.thermal")
